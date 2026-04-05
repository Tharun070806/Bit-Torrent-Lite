#include<iostream>
#include<string>
#include<vector>
#include <mutex>
#include<unordered_set>
#include <condition_variable>
#include <algorithm>
#include<optional>
#include <fstream>
#include<unordered_map>
#include<fstream>
#include <cstdio>   
#include <unistd.h>   
#include <fcntl.h>
#include "../utils/hasher.hpp"



/*
Main Job of Controller :
    1 -> Every peer(thread) will ask him which block should i request next, thus he gives them details (BLockRequest)
    2 -> When Peer gets a particular request, he will send it to Controller
        2.1 -> Controller checks whether the piece is there , if not there returns empty , if there then 
        2.2 -> If there then he reads the file from the disk and stores it in a string and gives it to the peer
    
*/

enum class Mode {
    LEECHER,
    SEEDER
};

struct BlockRequest {
    int piece_index;
    int block_offset;
    int block_length;
};

struct IncomingRequest {
    int piece_index;
    int block_offset;
    int block_length;
};

class Controller{
    public:

    Mode mode;
    std::string filename;
    int64_t length;
    int total_pieces;// 2^35 / 2^20 , thus this can go till 2^15 if the file is lets say 32 GB etc.. so integer is fine 
    int block_length=16384; //16kb is the convention approx 2^15
    int piece_length;// lets say 1MB then approx 2^20
    int crnt_peer=0;

    std::mutex mtx;
    std::condition_variable cv;
    //piece length can be till 1MB prolly 
    /*
    1KB-> 2^10
    1MB -> 2^20
    1GB -> 2^30 
    thus if the file is more than lets say 2gb , 3gb ,4gb etc then total length can be kept in int
    */
    std::vector<std::string>piece_hash;
    std::vector<std::unordered_set<int>>inflight;
    std::vector<int>piece_status; // 0-> not there , 1-> done , 2-> ongoing some pieces
    std:: vector<std::unordered_map<int,std::string>> piece_blocks;
    std::unordered_map<int,std::vector<int>> send_haves;
    std::vector<std::string> assembled_pieces;

    Controller(Mode mode, int total_pieces, int64_t length, int piece_length,std::vector<std::string>&piece_hash,std::string filename=""):
    mode(mode),
          total_pieces(total_pieces),
          piece_length(piece_length),
          length(length),
          piece_hash(piece_hash),
          filename(filename),
          piece_status(total_pieces, mode==Mode::SEEDER),
          piece_blocks(total_pieces),
          inflight(total_pieces),
          assembled_pieces(mode == Mode::LEECHER ? total_pieces : 0)
           {}


    std::vector<uint8_t> get_bitfield() {
        std::lock_guard<std::mutex> lock(mtx);
        int num_bytes = (total_pieces + 7) / 8;
        std::vector<uint8_t> bf(num_bytes, 0);
        for (int i = 0; i < total_pieces; i++)
            if (piece_status[i]==1)
                bf[i / 8] |= (1 << (7 - (i % 8)));
        return bf;
    }

    int piece_length_for(int i){
        if (i < total_pieces - 1) return piece_length;
        return 1ll*length - 1ll*piece_length * i;
    }

    int count_blocks(int piece_index){
        int piece_len  = piece_length_for(piece_index);
        int num_blocks = (piece_len + block_length - 1) / block_length;
    }

    // Each thread will ask for which piece to ask for next from the manager

    std::optional<BlockRequest> next_request(const std::vector<bool>bit_field){
        std::lock_guard<std::mutex> lock(mtx);
        if (mode == Mode::SEEDER) return std::nullopt;

        for (int i = 0; i < total_pieces; i++) {
            if (!bit_field[i]) continue;
            if (piece_status[i] == 1) continue;

            int num_blocks=count_blocks(i);
            int piece_len=piece_length_for(i);

            for (int b = 0; b < num_blocks; b++) {
                int offset = b * block_length; // should be less than piece length
                if (piece_blocks[i].count(offset)) continue;
                if (inflight[i].count(offset))    continue;

                inflight[i].insert(offset);
                piece_status[i] = 2;
                int len = std::min(block_length, piece_len - offset);
                return BlockRequest{i, offset, len};
            }
        }
        return std::nullopt;
    }

    bool all_done(){
        if (mode == Mode::SEEDER) return false; 
        std::lock_guard<std::mutex> lock(mtx);
        for(int i=0;i<total_pieces;i++){
            if(!piece_status[i]){
                return false;
            }
        }
        return true;
    }

    // check if seeder has the entire file( only for seeder )

    bool seederCheck(){
        if (mode != Mode::SEEDER) return false;
        std::ifstream f(filename, std::ios::binary);
        if(!f){
            std::cerr << "[Manager] cannot open "<<filename<<std::endl;
        }
        for(int i=0;i<total_pieces;i++){
            int plen = piece_length_for(i);
            std::string piece(plen, '\0');
            f.read(piece.data(), plen);
            if ((int)f.gcount() != plen) {
                std::cerr << "[Manager] short read on piece " << i << "\n";
                return false;
            }
            if(sha1(piece)!=piece_hash[i]){
                std::cerr << "[Manager] piece hash didnt match " << i << "\n";
                return false;
            }
        }

        std::cout << "[Manager] file verified: " << total_pieces
                  << " pieces from " << filename << "\n";
        return true;

    }

    //things to do when we receive a block:
    /* 
        1 -> store the block, remove it from inqueue request
        2 -> check if the piece is fuly received
            2.1-> if yes, then concatenate all the piece and check its hash
            2.2-> if hash is fine, then save it to disk
            2.3-> after saving the hash, delete all the block's data as its taking ram (unordered_map)
            2.4-> update that piece is received to the manager ( the manager has piece_status array )
            2.5-> we need to announce that this piece is received to all the other peers thus add this piece to all the peer's have message queue 
            2.6-> running and sending the request in the queue wil be taken care by someother function in the peer loop
    */

    bool received_block(int piece_index, int block_offset, const std::string &data){
        std::lock_guard<std::mutex> lock(mtx);
        if (mode == Mode::SEEDER) return false;

        inflight[piece_index].erase(block_offset);
        piece_blocks[piece_index][block_offset]=data;
        int num_block=count_blocks(piece_index);

        for(int i=0;i<num_block;i++){
            int offset=i*block_length;
            if(!piece_blocks[piece_index].count(offset)) return false;
        }

        std::string piece;
        piece.reserve(piece_length_for(piece_index));

        for( int i=0;i<num_block;i++){
            piece+=piece_blocks[piece_index][i*block_length];
        }

        if(sha1(piece)!=piece_hash[piece_index]) {
            std::cerr << "Piece is fully received but its hash not matching"<<std::endl;
            piece_blocks[piece_index].clear();
            return false;
        }

        write_to_disk( piece_index, piece);
        piece_status[piece_index]=1;

        for(int i=0;i<total_pieces;i++){
            if(piece_status[i]==0){
                return false;
            }
        }

        // this piece has to be announced to others //

        if(all_done()){
            cv.notify_all();
            return true;
        }
        else{
            return false;
        }

    }


    // Note Currently when the file size is large than 2GB there might be a problem of 

    void write_to_disk(int piece_index, std::string& piece){
        
        int64_t file_offset = (int64_t)piece_index * piece_length;
        // open for read+write without truncating
        std::fstream f(filename,
                       std::ios::binary | std::ios::in | std::ios::out);
        if (!f) {
            // file doesn't exist yet — create it at full size first
            std::ofstream create(filename, std::ios::binary);
            create.seekp(length - 1);
            create.put('\0'); // pre-allocate
            create.close();

            f.open(filename,
                   std::ios::binary | std::ios::in | std::ios::out);
        }

        f.seekp(file_offset);
        f.write(piece.data(), piece.size());
    } 

    //this function is used by while seeding when the block in the disk is retreived and is sent as a string 

    std::string get_block (int piece_index, int blockoffset){
        if (piece_status[piece_index]!=1) return "";

        int piece_len = piece_length_for(piece_index);
        if (blockoffset >= piece_len) return "";

        int actual_len = std::min(block_length, piece_len - blockoffset);
        int64_t file_offset = (int64_t)piece_index * piece_length + blockoffset;

        // read directly from disk — no RAM needed
        std::ifstream f(filename, std::ios::binary);
        if (!f) return "";
        f.seekg(file_offset);
        std::string buf(actual_len, '\0');
        f.read(buf.data(), actual_len);
        if ((int)f.gcount() != actual_len) return "";
        return buf;
    }

    void release_in_flight(int piece_index, int block_offset) {
        std::lock_guard<std::mutex> lock(mtx);
        inflight[piece_index].erase(block_offset);
    }

    int register_peer() {
        std::lock_guard<std::mutex> lock(mtx);
        int id = crnt_peer++;
        send_haves[id] = {};
        return id;
    }

    void unregister_peer(int peer_id) {
        std::lock_guard<std::mutex> lock(mtx);
        send_haves.erase(peer_id);
    }

    std::vector<int> drain_haves(int peer_id) {
        std::lock_guard<std::mutex> lock(mtx);
        auto& v = send_haves[peer_id];
        std::vector<int> result = std::move(v);
        v.clear();
        return result;
    }


    









    
};