#ifndef WIB2FRAME_HPP
#define WIB2FRAME_HPP

#include <stdint.h>  // For uint32_t etc
#include <algorithm> // For std::min
#include <stdexcept> // For std::out_of_range
#include <cassert>   // For assert()
/**
 *  @brief Class for accessing raw WIB v2 frames, as used in ProtoDUNE-SP-II
 *
 *  The canonical definition of the WIB format is given in EDMS document 2088713:
 *  https://edms.cern.ch/document/2088713/4
 */
class WIB2Frame
{
public:
    // ===============================================================
    // Preliminaries
    // ===============================================================

    // The definition of the format is in terms of 32-bit words
    typedef uint32_t word_t;

    struct Header
    {
        
        word_t crate : 8, frame_version : 4, slot : 3, fiber : 1, femb_valid : 2, wib_code_1 : 14;
        word_t wib_code_2 : 32;
        word_t timestamp_1 : 32;
        word_t timestamp_2 : 32;
    };

    struct Trailer
    {
        word_t crc20 : 20;
        word_t flex_word_12 : 12;
        word_t eof : 8;
        word_t flex_word_24 : 24;
    };


    // ===============================================================
    // Data members
    // ===============================================================
    Header header;
    word_t adc_words[112];
    Trailer trailer;

    // ===============================================================
    // Accessors
    // ===============================================================

    /**
     * @brief Get the ith ADC value in the frame
     *
     * The ADC words are 14 bits long, stored packed in the data structure. The order is:
     *
     * - 40 values from FEMB0 U channels
     * - 40 values from FEMB0 V channels
     * - 48 values from FEMB0 X channels (collection)
     * - 40 values from FEMB1 U channels
     * - 40 values from FEMB1 V channels
     * - 48 values from FEMB1 X channels (collection)
     */
    uint16_t get_adc(int i)
    {
        if(i<0 || i>255) throw std::out_of_range("ADC index out of range");

        constexpr int BITS_PER_ADC=14;
        constexpr int BITS_PER_WORD=8*sizeof(word_t);

        // The index of the first (and sometimes only) word containing the required ADC value
        int word_index=BITS_PER_ADC*i/BITS_PER_WORD;
        assert(word_index < 112);
        // Where in the word the lowest bit of our ADC value is located
        int first_bit_position=(BITS_PER_ADC*i)%BITS_PER_WORD;
        // How many bits of our desired ADC are located in the `word_index`th word
        int bits_from_first_word=std::min(BITS_PER_ADC, BITS_PER_WORD-first_bit_position);
        uint16_t adc=adc_words[word_index] >> first_bit_position;
        // If we didn't get the full 14 bits from this word, we need the rest from the next word
        if(bits_from_first_word < BITS_PER_WORD){
            assert(word_index+1 < 112);
            adc |= adc_words[word_index+1] << bits_from_first_word;
        }
        // Mask out all but the lowest 14 bits;
        return adc & 0x3FFFu;
    }

    /** @brief Get the ith U-channel ADC in the given femb
     */
    uint16_t get_u(int femb, int i) { return get_adc(128*femb + i); }
    /** @brief Get the ith V-channel ADC in the given femb
     */
    uint16_t get_v(int femb, int i) { return get_adc(128*femb + 40 + i); }
    /** @brief Get the ith X-channel (ie, collection) ADC in the given femb
     */
    uint16_t get_x(int femb, int i) { return get_adc(128*femb + 40 + 40 + i); }

    /** @brief Get the 64-bit timestamp of the frame
     */
    uint64_t timestamp() const {
        return header.timestamp_1 | ((uint64_t)header.timestamp_2 << 32);
    }

};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
