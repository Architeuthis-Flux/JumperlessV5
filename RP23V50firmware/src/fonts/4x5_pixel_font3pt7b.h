const uint8_t 4x5_pixel_font3pt7bBitmaps[] PROGMEM = {
  0x00, 0xE0, 0xC0, 0xEB, 0x80, 0x5D, 0xE0, 0xEA, 0x80, 0xDE, 0x80, 0x80,
  0xE4, 0xD8, 0xF0, 0xE8, 0x80, 0xE0, 0x80, 0x78, 0xFD, 0x00, 0xCB, 0x80,
  0xEB, 0x80, 0xFF, 0x00, 0xEF, 0x20, 0xFF, 0x00, 0xFD, 0x00, 0xEE, 0x00,
  0xFD, 0x00, 0xEF, 0x00, 0xC0, 0xA0, 0x71, 0x80, 0xFC, 0x8E, 0x00, 0xEA,
  0x00, 0xF1, 0x00, 0x5E, 0x80, 0xFF, 0x00, 0xF1, 0x80, 0xF7, 0x00, 0xFB,
  0x80, 0xFA, 0x00, 0xF5, 0x00, 0xBE, 0x80, 0xEB, 0x80, 0xFA, 0x00, 0xBA,
  0x80, 0x93, 0x80, 0xF6, 0x80, 0xFE, 0x80, 0xF5, 0x00, 0xFA, 0x00, 0xFD,
  0x80, 0xFB, 0x80, 0xEF, 0x00, 0xE9, 0x00, 0xB5, 0x00, 0xB8, 0x00, 0xBD,
  0x80, 0xE2, 0x80, 0xA9, 0x00, 0xFB, 0x80, 0xEC, 0xB4, 0xDC, 0x54, 0xE0,
  0x90, 0xD4, 0x9F, 0x00, 0xEC, 0x3D, 0x80, 0xEC, 0x7A, 0x00, 0xEF, 0x00,
  0x9A, 0x80, 0xE0, 0x5E, 0xBA, 0x80, 0xE0, 0x74, 0xD4, 0xE8, 0xFA, 0x00,
  0xEC, 0x80, 0xF0, 0x78, 0xE8, 0x80, 0xB5, 0x80, 0xA8, 0xEC, 0xF4, 0xAA,
  0x00, 0xCC, 0x74, 0xE0, 0xB8, 0x74 };

const GFXglyph 4x5_pixel_font3pt7bGlyphs[] PROGMEM = {
  {     0,   1,   1,   1,    0,    0 },   // 0x20 ' '
  {     1,   1,   3,   2,    0,   -2 },   // 0x21 '!'
  {     2,   2,   1,   3,    0,   -2 },   // 0x22 '"'
  {     3,   3,   3,   4,    0,   -2 },   // 0x23 '#'
  {     5,   3,   4,   4,    0,   -3 },   // 0x24 '$'
  {     7,   3,   3,   4,    0,   -2 },   // 0x25 '%'
  {     9,   3,   3,   4,    0,   -2 },   // 0x26 '&'
  {    11,   1,   1,   2,    0,   -2 },   // 0x27 '''
  {    12,   2,   3,   2,    0,   -2 },   // 0x28 '('
  {    13,   2,   3,   3,    0,   -2 },   // 0x29 ')'
  {    14,   2,   2,   3,    0,   -2 },   // 0x2A '*'
  {    15,   3,   2,   4,    0,   -1 },   // 0x2B '+'
  {    16,   1,   1,   2,    0,    0 },   // 0x2C ','
  {    17,   3,   1,   4,    0,   -1 },   // 0x2D '-'
  {    18,   1,   1,   1,    0,    0 },   // 0x2E '.'
  {    19,   2,   3,   3,    0,   -2 },   // 0x2F '/'
  {    20,   3,   3,   4,    0,   -2 },   // 0x30 '0'
  {    22,   3,   3,   4,    0,   -2 },   // 0x31 '1'
  {    24,   3,   3,   4,    0,   -2 },   // 0x32 '2'
  {    26,   3,   3,   4,    0,   -2 },   // 0x33 '3'
  {    28,   4,   3,   4,    0,   -2 },   // 0x34 '4'
  {    30,   3,   3,   4,    0,   -2 },   // 0x35 '5'
  {    32,   3,   3,   4,    0,   -2 },   // 0x36 '6'
  {    34,   3,   3,   4,    0,   -2 },   // 0x37 '7'
  {    36,   3,   3,   4,    0,   -2 },   // 0x38 '8'
  {    38,   3,   3,   4,    0,   -2 },   // 0x39 '9'
  {    40,   1,   2,   1,    0,   -2 },   // 0x3A ':'
  {    41,   1,   3,   2,    0,   -2 },   // 0x3B ';'
  {    42,   3,   3,   4,    0,   -2 },   // 0x3C '<'
  {    44,   3,   2,   4,    0,   -2 },   // 0x3D '='
  {    45,   3,   3,   4,    0,   -2 },   // 0x3E '>'
  {    47,   3,   3,   4,    0,   -2 },   // 0x3F '?'
  {    49,   3,   3,   4,    0,   -2 },   // 0x40 '@'
  {    51,   3,   3,   4,    0,   -2 },   // 0x41 'A'
  {    53,   3,   3,   4,    0,   -2 },   // 0x42 'B'
  {    55,   3,   3,   4,    0,   -2 },   // 0x43 'C'
  {    57,   3,   3,   4,    0,   -2 },   // 0x44 'D'
  {    59,   3,   3,   4,    0,   -2 },   // 0x45 'E'
  {    61,   3,   3,   4,    0,   -2 },   // 0x46 'F'
  {    63,   3,   3,   4,    0,   -2 },   // 0x47 'G'
  {    65,   3,   3,   4,    0,   -2 },   // 0x48 'H'
  {    67,   3,   3,   4,    0,   -2 },   // 0x49 'I'
  {    69,   3,   3,   3,    0,   -2 },   // 0x4A 'J'
  {    71,   3,   3,   4,    0,   -2 },   // 0x4B 'K'
  {    73,   3,   3,   4,    0,   -2 },   // 0x4C 'L'
  {    75,   3,   3,   4,    0,   -2 },   // 0x4D 'M'
  {    77,   3,   3,   4,    0,   -2 },   // 0x4E 'N'
  {    79,   3,   3,   4,    0,   -2 },   // 0x4F 'O'
  {    81,   3,   3,   4,    0,   -2 },   // 0x50 'P'
  {    83,   3,   3,   4,    0,   -2 },   // 0x51 'Q'
  {    85,   3,   3,   4,    0,   -2 },   // 0x52 'R'
  {    87,   3,   3,   4,    0,   -2 },   // 0x53 'S'
  {    89,   3,   3,   4,    0,   -2 },   // 0x54 'T'
  {    91,   3,   3,   4,    0,   -2 },   // 0x55 'U'
  {    93,   3,   3,   4,    0,   -2 },   // 0x56 'V'
  {    95,   3,   3,   4,    0,   -2 },   // 0x57 'W'
  {    97,   3,   3,   4,    0,   -2 },   // 0x58 'X'
  {    99,   3,   3,   4,    0,   -2 },   // 0x59 'Y'
  {   101,   3,   3,   4,    0,   -2 },   // 0x5A 'Z'
  {   103,   2,   3,   2,    0,   -2 },   // 0x5B '['
  {   104,   2,   3,   3,    0,   -2 },   // 0x5C '\'
  {   105,   2,   3,   3,    0,   -2 },   // 0x5D ']'
  {   106,   3,   2,   1,   -3,   -5 },   // 0x5E '^'
  {   107,   3,   1,   4,    0,    0 },   // 0x5F '_'
  {   108,   2,   2,   1,   -3,   -5 },   // 0x60 '`'
  {   109,   3,   2,   4,    0,   -1 },   // 0x61 'a'
  {   110,   3,   3,   4,    0,   -2 },   // 0x62 'b'
  {   112,   3,   2,   4,    0,   -1 },   // 0x63 'c'
  {   113,   3,   3,   4,    0,   -2 },   // 0x64 'd'
  {   115,   3,   2,   4,    0,   -1 },   // 0x65 'e'
  {   116,   3,   3,   4,    0,   -2 },   // 0x66 'f'
  {   118,   3,   3,   4,    0,   -1 },   // 0x67 'g'
  {   120,   3,   3,   4,    0,   -2 },   // 0x68 'h'
  {   122,   1,   3,   2,    0,   -2 },   // 0x69 'i'
  {   123,   2,   4,   3,    0,   -2 },   // 0x6A 'j'
  {   124,   3,   3,   4,    0,   -2 },   // 0x6B 'k'
  {   126,   1,   3,   2,    0,   -2 },   // 0x6C 'l'
  {   127,   3,   2,   4,    0,   -1 },   // 0x6D 'm'
  {   128,   3,   2,   4,    0,   -1 },   // 0x6E 'n'
  {   129,   3,   2,   4,    0,   -1 },   // 0x6F 'o'
  {   130,   3,   3,   4,    0,   -1 },   // 0x70 'p'
  {   132,   3,   3,   4,    0,   -1 },   // 0x71 'q'
  {   134,   3,   2,   4,    0,   -1 },   // 0x72 'r'
  {   135,   3,   2,   3,    0,   -1 },   // 0x73 's'
  {   136,   3,   3,   4,    0,   -2 },   // 0x74 't'
  {   138,   3,   3,   4,    0,   -2 },   // 0x75 'u'
  {   140,   3,   2,   4,    0,   -1 },   // 0x76 'v'
  {   141,   3,   2,   4,    0,   -1 },   // 0x77 'w'
  {   142,   3,   2,   4,    0,   -1 },   // 0x78 'x'
  {   143,   3,   3,   4,    0,   -1 },   // 0x79 'y'
  {   145,   3,   2,   3,    0,   -1 },   // 0x7A 'z'
  {   146,   2,   3,   3,    0,   -2 },   // 0x7B '{'
  {   147,   1,   3,   2,    0,   -2 },   // 0x7C '|'
  {   148,   2,   3,   3,    0,   -2 },   // 0x7D '}'
  {   149,   3,   2,   1,   -4,   -5 } }; // 0x7E '~'

const GFXfont 4x5_pixel_font3pt7b PROGMEM = {
  (uint8_t  *)4x5_pixel_font3pt7bBitmaps,
  (GFXglyph *)4x5_pixel_font3pt7bGlyphs,
  0x20, 0x7E, 9 };

// Approx. 822 bytes
