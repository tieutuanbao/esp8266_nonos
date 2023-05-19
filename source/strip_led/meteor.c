#include <meteor.h>

/**
 * @brief Hàm vẽ hiệu ứng Sao băng ra mảng color
 * 
 * @param dev           : Con trỏ đến hiệu ứng cần thêm màu
 * @param color_buf     : Con trỏ đến buffer color cần vẽ
 * @param color_buf_len : Kích thước của buffer
 * @param current_time  : Thời gian hiện tại, sử dụng thời gian này để cập nhật animation hiệu ứng
 * @param offset_pos    : Offset vị trí in hiệu ứng
 * @return draw_stt_t   : Trạng thái của hiệu ứng
 */
ICACHE_FLASH_ATTR void meteor_draw(Color_RGB_t *color_buf, uint16_t color_buf_len, int32_t offset_pos, strip_color_t *colors, uint8_t colors_size) {
    int32_t idx_effect_node = 0;
    int32_t idx_node_full_strip = 0;
    /**
     * @brief Giảm độ sáng toàn bộ điểm ảnh 
     */
    #define METEOR_DECAY_RATE   2
    // for(idx_effect_node = 0; idx_effect_node < color_buf_len; idx_effect_node++){
    //     if(color_buf[idx_effect_node].R >= METEOR_DECAY_RATE) color_buf[idx_effect_node].R -= METEOR_DECAY_RATE;
    //     else color_buf[idx_effect_node].R = 0;
    //     if(color_buf[idx_effect_node].G >= METEOR_DECAY_RATE) color_buf[idx_effect_node].G -= METEOR_DECAY_RATE;
    //     else color_buf[idx_effect_node].G = 0;
    //     if(color_buf[idx_effect_node].B >= METEOR_DECAY_RATE) color_buf[idx_effect_node].B -= METEOR_DECAY_RATE;
    //     else color_buf[idx_effect_node].B = 0;
    // }
    #undef METEOR_DECAY_RATE
    /* In từng dải màu */
    for(uint8_t idx_color_list = 0; idx_color_list < colors_size; idx_color_list++) {
        /* In ra giải màu theo offset */
        for(idx_effect_node = 0; idx_effect_node < colors[idx_color_list].len; idx_effect_node++){
            /* Lấy màu source và dest tạo hiệu ứng fade strip */
            strip_color_t source_color = colors[idx_color_list];
            strip_color_t dest_color = ((idx_color_list + 1) == colors_size)? (strip_color_t){{0, 0, 0}, 1} : colors[idx_color_list + 1];
            if(((offset_pos + idx_node_full_strip) >= 0) && ((offset_pos + idx_node_full_strip) < color_buf_len)){
                color_buf[offset_pos + idx_node_full_strip] = source_color.value;
                /* In màu */
                if(source_color.value.R < dest_color.value.R){
                    color_buf[offset_pos + idx_node_full_strip].R = source_color.value.R + (idx_effect_node * (dest_color.value.R - source_color.value.R)) / source_color.len;
                }
                else if(source_color.value.R > dest_color.value.R){
                    color_buf[offset_pos + idx_node_full_strip].R = source_color.value.R - (idx_effect_node * (source_color.value.R - dest_color.value.R)) / source_color.len;
                }
                if(source_color.value.G < dest_color.value.G){
                    color_buf[offset_pos + idx_node_full_strip].G = source_color.value.G + (idx_effect_node * (dest_color.value.G - source_color.value.G)) / source_color.len;
                }
                else if(source_color.value.G > dest_color.value.G){
                    color_buf[offset_pos + idx_node_full_strip].G = source_color.value.G - (idx_effect_node * (source_color.value.G - dest_color.value.G)) / source_color.len;
                }
                if(source_color.value.B < dest_color.value.B){
                    color_buf[offset_pos + idx_node_full_strip].B = source_color.value.B + (idx_effect_node * (dest_color.value.B - source_color.value.B)) / source_color.len;
                }
                else if(source_color.value.B > dest_color.value.B){
                    color_buf[offset_pos + idx_node_full_strip].B = source_color.value.B - (idx_effect_node * (source_color.value.B - dest_color.value.B)) / source_color.len;
                }
                /* Hiệu ứng phân rã */
                if(idx_color_list == 0){
                    if((rand() % 2) == 0) {
                        // if(color_buf[offset_pos + idx_node_full_strip].R) color_buf[offset_pos + idx_node_full_strip].R /= 5;
                        // if(color_buf[offset_pos + idx_node_full_strip].G) color_buf[offset_pos + idx_node_full_strip].G /= 5;
                        // if(color_buf[offset_pos + idx_node_full_strip].B) color_buf[offset_pos + idx_node_full_strip].B /= 5;
                        color_buf[offset_pos + idx_node_full_strip].R = 0;
                        color_buf[offset_pos + idx_node_full_strip].G = 0;
                        color_buf[offset_pos + idx_node_full_strip].B = 0;
                    }
                }
            }
            // BITS_LOGD("buf: {%d, %d, %d}, src: {%d, %d, %d}, dest: {%d, %d, %d}, idx_pos_node: %d, pos_show: %d\n", color_buf[offset_pos + idx_node_full_strip].R, color_buf[offset_pos + idx_node_full_strip].G , color_buf[offset_pos + idx_node_full_strip].B, source_color.value.R, source_color.value.G, source_color.value.B, dest_color.value.R, dest_color.value.G, dest_color.value.B, idx_effect_node, offset_pos + idx_node_full_strip);
            /* Vị trí in ra nhỏ hơn 0 thì bỏ qua */
            idx_node_full_strip++;
        }
    }
}