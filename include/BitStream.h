#pragma once

#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <stdexcept>


class BitStream {

public:
    BitStream(const std::string& base64_str, const std::string& type_) : type(type_){
        offset = 0;
        data = nullptr;
        lengthInBytes = base64_decode(base64_str, data);
        length = lengthInBytes * 8;

        if(data==nullptr){
            std::cerr << "ATTENZIONE errore nell'allocazione dello spazio" << std::endl;
        }
        if(lengthInBytes==0){
            std::cerr << "Passed base64 string generated no output" << std::endl;
            if(data != nullptr){
                free(data);            
                data = nullptr;
            }        
        }
    }

    BitStream(const unsigned char* src, size_t length_, const std::string& type_) : type(type_){
        if(length_==0){
            std::cerr << "Passed length is zero" << std::endl;
            data = nullptr;
            throw std::invalid_argument("Trying to create a zero length bit stream");
        }
        offset = 0;
        length = length_;
        lengthInBytes = (length + 7) >> 3;  // Allocate at least 1 byte

        data = static_cast<unsigned char*>(calloc(lengthInBytes, sizeof(unsigned char)));
        if(data==nullptr){
            std::cerr << "ATTENZIONE errore nell'allocazione dello spazio" << std::endl;
        }
        memcpy(data, src, lengthInBytes);
    }

    ~BitStream() {
        if(data != nullptr){
            free(data);            
            data = nullptr;
        }
    }

    BitStream read(int length_) {
        if(offset + length_ > length){
            throw std::length_error("Trying to access more bits than provided: current offset is <" + 
                                    std::to_string(offset) + ">, bits to be consumed are <" +
                                    std::to_string(length_) + ">, total amount of bits is <" + std::to_string(length) + ">");
        }

        int byte_offset = offset >> 3;
        int bit_offset = offset % 8;
        size_t num_blocks = (length_ + bit_offset + 7) >> 3;
        unsigned char result[num_blocks];

        //cut the relevant part of the stream
        int bits_to_extract = length_;
        for(int i = 0; i < num_blocks && bits_to_extract > 0; i++) { 
            int bits_remaining = bits_to_extract + bit_offset > 8 ? 8 - bit_offset : bits_to_extract;
            unsigned short mask = ((1 << bits_remaining) - 1) << (8-bits_remaining-bit_offset);
            unsigned char extracted_bits = (data[byte_offset] & mask) >> 8-bits_remaining-bit_offset;
            result[i] = extracted_bits;
            bits_to_extract -= bits_remaining;
            byte_offset++;
            bit_offset = 0;
        }

        unsigned int alignment = (length_ + offset) % 8;
        if(offset % 8 || alignment){
            //align all the bytes to the right (the first one is already aligned)
            unsigned short bits_remaining = 8 - alignment;
            unsigned short mask = ((1 << bits_remaining) - 1);
            for(int i = num_blocks-1-1; i >= 0; i--){
                unsigned char carry = (result[i] & mask); 
                result[i] = result[i] >> bits_remaining;
                result[i+1] = result[i+1] | (carry << alignment);
            }
        }

        BitStream bt;
        size_t real_num_blocks = (length_ + 7) >> 3;
        if(real_num_blocks == num_blocks){
            bt.setBitStream(result, length_, "");
        } else {
            //cut not needed bytes
            unsigned char real_result[real_num_blocks];
            for(int i = real_num_blocks-1; i >= 0; i--){
                unsigned int index = i + (num_blocks-real_num_blocks);
                real_result[i] = result[index];
            }         
            bt.setBitStream(real_result, length_, "");            
        }
        return bt;
    }

    BitStream consume(int length_) {
        BitStream bt = read(length_);
        offset += length_;
        return bt;
    }    

    BitStream consumeUntill(int delimiter){
        BitStream bt;
        unsigned int offset_ = offset;
        while(readFrom(8, offset_).to_int()!=delimiter && offset_<length){
            offset_++;
        }
        if(offset_>=length){
            return bt;
        }
        return consume(offset_-offset);
    }


    static BitStream combine(BitStream& a, BitStream& b){
        unsigned int totalLength = a.length + b.length;
        unsigned int totalLengthInBytes = (totalLength + 7) >> 3;
        unsigned char result[totalLengthInBytes];

        // Copy BitStream b at the end of the resulting BitStream
        memset(result, 0, totalLengthInBytes*sizeof(unsigned char));
        memcpy(result + totalLengthInBytes - b.lengthInBytes, b.data, b.lengthInBytes);

        unsigned int alignment = b.lengthInBytes * 8 - b.length;
        if(alignment){
            // Align all the bytes of BitStream 'a' to the right
            // and copy them into the resulting BitStream
            unsigned short shifting = 8 - alignment;
            unsigned short mask = ((1 << alignment) - 1);
            for(int i = a.lengthInBytes-1; i > 0; i--){
                result[i-1] = a.data[i] >> alignment;
                result[i] = a.data[i] << shifting;
            }
        } else {
            memcpy(result, a.data, a.lengthInBytes);
        }

        BitStream bt(result, totalLength, "");
        return bt;
    }

    BitStream* append(BitStream* b){
        if(data==nullptr || length==0){
            data = static_cast<unsigned char*>(calloc(lengthInBytes, sizeof(unsigned char)));
            if(data==nullptr){
                std::cerr << "ATTENZIONE errore nell'allocazione dello spazio" << std::endl;
            }
            unsigned int alignment = b->length % 8;
            unsigned char shifted[b->lengthInBytes];
            if(alignment){
                unsigned short shifting = 8 - alignment;
                unsigned short mask = ((1 << shifting) - 1) << alignment;
                shifted[b->lengthInBytes-1] = b->data[b->lengthInBytes-1] << shifting;
                for(int i=b->lengthInBytes-1-1; i>=0; i--){
                    unsigned char carry = (b->data[i] & mask) >> alignment; 
                    shifted[i+1] |= carry;
                    shifted[i] = b->data[i] << shifting;
                }
                memcpy(data, shifted, b->lengthInBytes);
            } else {
                memcpy(data, b->data, b->lengthInBytes);
            }
            offset = 0;
            length = b->length;
            lengthInBytes = b->lengthInBytes;
            return this;
        }

        unsigned int totalLength = length + b->length;
        unsigned int totalLengthInBytes = (totalLength + 7) >> 3;
        unsigned char* result = static_cast<unsigned char*>(calloc(totalLengthInBytes, sizeof(unsigned char)));
        if(result==nullptr){
            std::cerr << "ATTENZIONE errore nell'allocazione dello spazio" << std::endl;
        }

        memcpy(result, data, lengthInBytes);

        // Considering the possibility that the bitstream 
        // to be appended is not a multiple of 8 bits
        unsigned int alignment = b->length % 8;
        unsigned char shifted[b->lengthInBytes];
        if(alignment){
            unsigned short shifting = 8 - alignment;
            unsigned short mask = ((1 << shifting) - 1) << alignment;
            shifted[b->lengthInBytes-1] = b->data[b->lengthInBytes-1] << shifting;
            for(int i=b->lengthInBytes-1-1; i>=0; i--){
                unsigned char carry = (b->data[i] & mask) >> alignment; 
                shifted[i+1] |= carry;
                shifted[i] = b->data[i] << shifting;
            }
        } else {
            memcpy(shifted, b->data, b->lengthInBytes);
        }

        // Considering the possibility that the destination
        // bitstream is not a multiple of 8 bits
        alignment = length % 8;
        if(alignment){
            unsigned short shifting = 8 - alignment;
            unsigned short mask = ((1 << shifting) - 1) << alignment;
            for(int i=lengthInBytes-1, index=b->lengthInBytes-1; i<totalLengthInBytes-1 && index>=0; i++, index--){
                result[i] |= (shifted[index] & mask) >> alignment;
                unsigned char carry = (shifted[index] & ~mask) << shifting; 
                result[i+1] |= carry;
            }
            result[totalLengthInBytes-1] |= (shifted[0] & mask) >> alignment;
        } else {
            memcpy(result + lengthInBytes, shifted, b->lengthInBytes);
        }

        if(data!=nullptr){
           free(data);
        }
        data = result;
        length = totalLength;
        lengthInBytes = totalLengthInBytes;
        offset = 0;
        return this;
    }


    BitStream* shift(int n) { 
        offset += n;
        return this;
    }

    BitStream* reset() {
        offset = 0;
        return this;
    }

    int to_int_bcd(size_t bits = 4){
        if(bits%4){ std::cout << "Unsupported number of bits for BCD encoding: " << bits << std::endl; }
        int8_t value[8];
        unsigned int mult = bits >> 2;
        std::memcpy(value, data, mult*sizeof(int8_t));
        int result = 0;
        unsigned short mask_low  = 0b00001111;
        unsigned short mask_high = 0b11110000;
        for (int i = mult-1; i >= 0; i--) {
            if(i%2){ result += (value[i] & mask_low) * (i*10); }
            else { result += ((value[i] & mask_high) >> 4) * (i*10); }
        }
        return result;
    }

    int to_int(size_t bits = 8){
        int8_t value[8];
        if(bits<=8){ bits=8; }
        else if(bits<=16){ bits=16; }
        else if(bits<=32){ bits=32; }
        else if(bits<=64){ bits=64; }
        else { std::cout << "Unsupported number of bits for integer " << bits << std::endl; }
        int mult = bits >> 3;
        std::memcpy(value, data, mult*sizeof(int8_t));
        int result = 0;
        ///???BIG ENDIAN LITTLE ENDIAN
//        for (int i = 0; i < mult; i++) {
//            result |= value[i] << (i * 8);
        for (int i = mult-1; i >= 0; i--) {
            result |= value[mult-1-i] << (i * 8);
        }
        return result;
    }
    int to_int8(){ return to_int(8); }
    int to_int16(){ return to_int(16); }
    int to_int32(){ return to_int(32); }
    int to_int64(){ return to_int(64); }

    unsigned int to_uint(size_t bits = 8){
        uint8_t value[8];
        if(bits<=8){ bits=8; }
        else if(bits<=16){ bits=16; }
        else if(bits<=32){ bits=32; }
        else if(bits<=64){ bits=64; }
        else { std::cout << "Unsupported number of bits" << std::endl; }
        int mult = bits >> 3;
        std::memcpy(value, data, mult*sizeof(uint8_t));
        unsigned int result = 0;
        for (int i = mult-1; i >= 0; i--) {
            result |= value[mult-1-i] << (i * 8);
        }
        return result;
    }
    unsigned int to_uint8(){ return to_uint(8); }
    unsigned int to_uint16(){ return to_uint(16); }
    unsigned int to_uint32(){ return to_uint(32); }
    unsigned int to_uint64(){ return to_uint(64); }

    std::string to_string(){
        return std::string(reinterpret_cast<char*>(data), lengthInBytes);
    }

    double to_double(size_t bits = 32){
        switch(bits){
            case 32:
                uint8_t value32[4];
                std::memcpy(value32, data, 4*sizeof(uint8_t));
                std::swap(value32[0], value32[3]);
                std::swap(value32[1], value32[2]);
                return *reinterpret_cast<double*>(value32);
                break;
            case 64:
                uint8_t value64[8];
                std::memcpy(value64, data, 8*sizeof(uint8_t));
                std::swap(value64[0], value64[7]);
                std::swap(value64[1], value64[6]);
                std::swap(value64[2], value64[5]);
                std::swap(value64[3], value64[4]);
                return *reinterpret_cast<double*>(value64);
                break;
            default:
                std::cout << "Unsupported number of bits" << std::endl; 
                break;
        }
        return 0.0;
    }
    double to_double16(){ return to_double(16); }
    double to_double64(){ return to_double(64); }

    bool to_boolean(){
        for(size_t i=0; i<lengthInBytes; i++){
            if(data[i]){
                return true;
            }
        }
        return false;
    }

    const unsigned int getOffset(){return offset;}
    const unsigned char* getData(){return data;}
    const unsigned int getLength(){return length;}
    const unsigned int getLengthInBytes(){return lengthInBytes;}

    const std::string getType() const {return type;}

    const std::string toBase64() {
        if(lengthInBytes==0 || data==nullptr){ return ""; }
        unsigned char clearData[lengthInBytes];
        memcpy(clearData, data, lengthInBytes*sizeof(unsigned char));
        int alignment = 8 - (length%8);
        if(alignment){
            unsigned char mask = ((1 << alignment) - 1);
            clearData[lengthInBytes-1] = data[lengthInBytes-1] & ~mask;
        }
        return base64_encode(clearData, lengthInBytes);
    }

    const std::string toString() {
        std::ostringstream oss;
        for(int i = 0; i < lengthInBytes; i++){
            for (int j = 7; j >= 0; j--) {
                oss << ((data[i] >> j) & 1);
            }
        }
        oss << " - " << length << " bit(s)";
        return oss.str();
    }
    friend std::ostream& operator<<(std::ostream& os, const BitStream& obj) {
        for(int i = 0; i < obj.lengthInBytes; i++){
            for (int j = 7; j >= 0; j--) {
                os << ((obj.data[i] >> j) & 1);
            }
        }
        return os;
    }

    BitStream(const BitStream& source) {
        type = source.type;
        offset = source.offset;
        length = source.length;
        lengthInBytes = source.lengthInBytes;

        data = static_cast<unsigned char*>(calloc(lengthInBytes, sizeof(unsigned char)));
        if(data==nullptr){
            std::cerr << "ATTENZIONE errore nell'allocazione dello spazio" << std::endl;
        }
        memcpy(data, source.data, lengthInBytes);
    }

    int remainingBits(){
        int remainingBits = length-offset-1;
        if(remainingBits<0) return 0;
        return remainingBits;
    }

    BitStream(){
        offset = 0;
        data = nullptr;
        lengthInBytes = 0;
        length = 0;
        type = "<undefined>";
    }


private:
    std::string type;
    unsigned char* data;
    unsigned int offset;
    unsigned int length;
    unsigned int lengthInBytes;


    void setBitStream(const unsigned char* src, size_t length_, const std::string& type_){
        if(length_==0){
            std::cerr << "Passed length is zero" << std::endl;
            data = nullptr;
            throw std::invalid_argument("Trying to create a zero length bit stream");
        }
        offset = 0;
        length = length_;
        type = type_;
        lengthInBytes = (length + 7) >> 3; 

        if(data != nullptr){
            free(data);            
            data = nullptr;
        }
        data = static_cast<unsigned char*>(calloc(lengthInBytes, sizeof(unsigned char)));
        if(data==nullptr){
            std::cerr << "ATTENZIONE errore nell'allocazione dello spazio" << std::endl;
        }
        memcpy(data, src, lengthInBytes);
    }


    BitStream readFrom(int length_, int offset_) {
        if(offset_ + length_ > length){
            throw std::length_error("Trying to access more bits than provided: current offset is <" + 
                                    std::to_string(offset_) + ">, bits to be consumed are <" +
                                    std::to_string(length_) + ">, total amount of bits is <" + std::to_string(length) + ">");
        }

        int byte_offset = offset_ >> 3;
        int bit_offset = offset_ % 8;
        size_t num_blocks = (length_ + bit_offset + 7) >> 3;
        unsigned char result[num_blocks];

        //cut the relevant part of the stream
        int bits_to_extract = length_;
        for(int i = 0; i < num_blocks && bits_to_extract > 0; i++) { 
            int bits_remaining = bits_to_extract + bit_offset > 8 ? 8 - bit_offset : bits_to_extract;
            unsigned short mask = ((1 << bits_remaining) - 1) << (8-bits_remaining-bit_offset);
            unsigned char extracted_bits = (data[byte_offset] & mask) >> 8-bits_remaining-bit_offset;
            result[i] = extracted_bits;
            bits_to_extract -= bits_remaining;
            byte_offset++;
            bit_offset = 0;
        }

        unsigned int alignment = (length_ + offset_) % 8;
        if(offset_ % 8 || alignment){
            //align all the bytes to the right (the first one is already aligned)
            unsigned short bits_remaining = 8 - alignment;
            unsigned short mask = ((1 << bits_remaining) - 1);
            for(int i = num_blocks-1-1; i >= 0; i--){
                unsigned char carry = (result[i] & mask); 
                result[i] = result[i] >> bits_remaining;
                result[i+1] = result[i+1] | (carry << alignment);
            }
        }

        BitStream bt;
        size_t real_num_blocks = (length_ + 7) >> 3;
        if(real_num_blocks == num_blocks){
            bt.setBitStream(result, length_, "");
        } else {
            //cut not needed bytes
            unsigned char real_result[real_num_blocks];
            for(int i = real_num_blocks-1; i >= 0; i--){
                unsigned int index = i + (num_blocks-real_num_blocks);
                real_result[i] = result[index];
            }         
            bt.setBitStream(real_result, length_, "");            
        }
        return bt;
    }
    static constexpr const char base64_chars[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    static inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

    static size_t base64_decode(const std::string& encoded_string, unsigned char*& decoded_data) {
        size_t in_len = encoded_string.size();
        size_t i = 0;
        size_t j = 0;
        size_t in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        int k = 0;

        if(decoded_data!=nullptr){
            free(decoded_data);
        }
        decoded_data = reinterpret_cast<unsigned char*>(calloc((in_len * 3) / 4 + 1, sizeof(unsigned char)));

        while (in_len-- && (encoded_string[in_] != '=') && (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = std::find(BitStream::base64_chars, BitStream::base64_chars + 64, char_array_4[i]) - BitStream::base64_chars;

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; i < 3; i++)
                    decoded_data[k++] = char_array_3[i];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = std::find(BitStream::base64_chars, BitStream::base64_chars + 64, char_array_4[j]) - BitStream::base64_chars;

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++)
                decoded_data[k++] = char_array_3[j];
        }
        return static_cast<size_t>(k);
    }

    static const std::string base64_encode(unsigned char* data, size_t length) {
        std::string encoded_string;
        size_t length_ = length;
        unsigned char* data_ = data;

        while (length_>0) {
            unsigned char input_array[3] = {0};
            int padding = 0;

            input_array[0] = *(data_++);
            length_--;
            if (length_) {
                input_array[1] = *(data_++);
                length_--;
                padding++;
            }
            if (length_) {
                input_array[2] = *(data_++);
                length_--;
                padding++;
            }

            encoded_string += base64_chars[(input_array[0] & 0xfc) >> 2];
            encoded_string += base64_chars[((input_array[0] & 0x03) << 4) | ((input_array[1] & 0xf0) >> 4)];
            encoded_string += padding ? base64_chars[((input_array[1] & 0x0f) << 2) | ((input_array[2] & 0xc0) >> 6)] : '=';
            encoded_string += padding ? base64_chars[input_array[2] & 0x3f] : '=';
        }

        return encoded_string;
    }
};



