#ifndef LCD_H
#define LCD_H

void lcd_init(void);

void lcd_write_begin(void);
void lcd_write_data(uint16_t value);

#endif
