static const uint32_t to_ucs4[256] = {
  [0x01] = 0x0001,
  [0x02] = 0x0002,
  [0x03] = 0x0003,
  [0x04] = 0x0004,
  [0x05] = 0x0005,
  [0x06] = 0x0006,
  [0x07] = 0x0007,
  [0x08] = 0x0008,
  [0x09] = 0x0009,
  [0x0a] = 0x000A,
  [0x0b] = 0x000B,
  [0x0c] = 0x000C,
  [0x0d] = 0x000D,
  [0x0e] = 0x000E,
  [0x0f] = 0x000F,
  [0x10] = 0x0010,
  [0x11] = 0x0011,
  [0x12] = 0x0012,
  [0x13] = 0x0013,
  [0x14] = 0x0014,
  [0x15] = 0x0015,
  [0x16] = 0x0016,
  [0x17] = 0x0017,
  [0x18] = 0x0018,
  [0x19] = 0x0019,
  [0x1a] = 0x001A,
  [0x1b] = 0x001B,
  [0x1c] = 0x001C,
  [0x1d] = 0x001D,
  [0x1e] = 0x001E,
  [0x1f] = 0x001F,
  [0x20] = 0x0020,
  [0x21] = 0x0021,
  [0x22] = 0x0022,
  [0x23] = 0x0023,
  [0x24] = 0x0024,
  [0x25] = 0x0025,
  [0x26] = 0x0026,
  [0x27] = 0x0027,
  [0x28] = 0x0028,
  [0x29] = 0x0029,
  [0x2a] = 0x002A,
  [0x2b] = 0x002B,
  [0x2c] = 0x002C,
  [0x2d] = 0x002D,
  [0x2e] = 0x002E,
  [0x2f] = 0x002F,
  [0x30] = 0x0030,
  [0x31] = 0x0031,
  [0x32] = 0x0032,
  [0x33] = 0x0033,
  [0x34] = 0x0034,
  [0x35] = 0x0035,
  [0x36] = 0x0036,
  [0x37] = 0x0037,
  [0x38] = 0x0038,
  [0x39] = 0x0039,
  [0x3a] = 0x003A,
  [0x3b] = 0x003B,
  [0x3c] = 0x003C,
  [0x3d] = 0x003D,
  [0x3e] = 0x003E,
  [0x3f] = 0x003F,
  [0x40] = 0x0040,
  [0x41] = 0x0041,
  [0x42] = 0x0042,
  [0x43] = 0x0043,
  [0x44] = 0x0044,
  [0x45] = 0x0045,
  [0x46] = 0x0046,
  [0x47] = 0x0047,
  [0x48] = 0x0048,
  [0x49] = 0x0049,
  [0x4a] = 0x004A,
  [0x4b] = 0x004B,
  [0x4c] = 0x004C,
  [0x4d] = 0x004D,
  [0x4e] = 0x004E,
  [0x4f] = 0x004F,
  [0x50] = 0x0050,
  [0x51] = 0x0051,
  [0x52] = 0x0052,
  [0x53] = 0x0053,
  [0x54] = 0x0054,
  [0x55] = 0x0055,
  [0x56] = 0x0056,
  [0x57] = 0x0057,
  [0x58] = 0x0058,
  [0x59] = 0x0059,
  [0x5a] = 0x005A,
  [0x5b] = 0x005B,
  [0x5c] = 0x005C,
  [0x5d] = 0x005D,
  [0x5e] = 0x005E,
  [0x5f] = 0x005F,
  [0x60] = 0x0060,
  [0x61] = 0x0061,
  [0x62] = 0x0062,
  [0x63] = 0x0063,
  [0x64] = 0x0064,
  [0x65] = 0x0065,
  [0x66] = 0x0066,
  [0x67] = 0x0067,
  [0x68] = 0x0068,
  [0x69] = 0x0069,
  [0x6a] = 0x006A,
  [0x6b] = 0x006B,
  [0x6c] = 0x006C,
  [0x6d] = 0x006D,
  [0x6e] = 0x006E,
  [0x6f] = 0x006F,
  [0x70] = 0x0070,
  [0x71] = 0x0071,
  [0x72] = 0x0072,
  [0x73] = 0x0073,
  [0x74] = 0x0074,
  [0x75] = 0x0075,
  [0x76] = 0x0076,
  [0x77] = 0x0077,
  [0x78] = 0x0078,
  [0x79] = 0x0079,
  [0x7a] = 0x007A,
  [0x7b] = 0x007B,
  [0x7c] = 0x007C,
  [0x7d] = 0x007D,
  [0x7e] = 0x007E,
  [0x7f] = 0x007F,
  [0x80] = 0x010C,
  [0x81] = 0x00FC,
  [0x82] = 0x0117,
  [0x83] = 0x0101,
  [0x84] = 0x00E4,
  [0x85] = 0x0105,
  [0x86] = 0x013C,
  [0x87] = 0x010D,
  [0x88] = 0x0113,
  [0x89] = 0x0112,
  [0x8a] = 0x0119,
  [0x8b] = 0x0118,
  [0x8c] = 0x012B,
  [0x8d] = 0x012F,
  [0x8e] = 0x00C4,
  [0x8f] = 0x0104,
  [0x90] = 0x0116,
  [0x91] = 0x017E,
  [0x92] = 0x017D,
  [0x93] = 0x00F5,
  [0x94] = 0x00F6,
  [0x95] = 0x00D5,
  [0x96] = 0x016B,
  [0x97] = 0x0173,
  [0x98] = 0x0123,
  [0x99] = 0x00D6,
  [0x9a] = 0x00DC,
  [0x9b] = 0x00A2,
  [0x9c] = 0x013B,
  [0x9d] = 0x201E,
  [0x9e] = 0x0161,
  [0x9f] = 0x0160,
  [0xa0] = 0x0100,
  [0xa1] = 0x012A,
  [0xa2] = 0x0137,
  [0xa3] = 0x0136,
  [0xa4] = 0x0146,
  [0xa5] = 0x0145,
  [0xa6] = 0x016A,
  [0xa7] = 0x0172,
  [0xa8] = 0x0122,
  [0xa9] = 0x2310,
  [0xaa] = 0x00AC,
  [0xab] = 0x00BD,
  [0xac] = 0x00BC,
  [0xad] = 0x012E,
  [0xae] = 0x00AB,
  [0xaf] = 0x00BB,
  [0xb0] = 0x2591,
  [0xb1] = 0x2592,
  [0xb2] = 0x2593,
  [0xb3] = 0x2502,
  [0xb4] = 0x2524,
  [0xb5] = 0x2561,
  [0xb6] = 0x2562,
  [0xb7] = 0x2556,
  [0xb8] = 0x2555,
  [0xb9] = 0x2563,
  [0xba] = 0x2551,
  [0xbb] = 0x2557,
  [0xbc] = 0x255D,
  [0xbd] = 0x255C,
  [0xbe] = 0x255B,
  [0xbf] = 0x2510,
  [0xc0] = 0x2514,
  [0xc1] = 0x2534,
  [0xc2] = 0x252C,
  [0xc3] = 0x251C,
  [0xc4] = 0x2500,
  [0xc5] = 0x253C,
  [0xc6] = 0x255E,
  [0xc7] = 0x255F,
  [0xc8] = 0x255A,
  [0xc9] = 0x2554,
  [0xca] = 0x2569,
  [0xcb] = 0x2566,
  [0xcc] = 0x2560,
  [0xcd] = 0x2550,
  [0xce] = 0x256C,
  [0xcf] = 0x2567,
  [0xd0] = 0x2568,
  [0xd1] = 0x2564,
  [0xd2] = 0x2565,
  [0xd3] = 0x2559,
  [0xd4] = 0x2558,
  [0xd5] = 0x2552,
  [0xd6] = 0x2553,
  [0xd7] = 0x256B,
  [0xd8] = 0x256A,
  [0xd9] = 0x2518,
  [0xda] = 0x250C,
  [0xdb] = 0x2588,
  [0xdc] = 0x2584,
  [0xdd] = 0x258C,
  [0xde] = 0x2590,
  [0xdf] = 0x2580,
  [0xe0] = 0x03B1,
  [0xe1] = 0x00DF,
  [0xe2] = 0x0393,
  [0xe3] = 0x03C0,
  [0xe4] = 0x03A3,
  [0xe5] = 0x03C3,
  [0xe6] = 0x00B5,
  [0xe7] = 0x03C4,
  [0xe8] = 0x03A6,
  [0xe9] = 0x0398,
  [0xea] = 0x03A9,
  [0xeb] = 0x03B4,
  [0xec] = 0x221E,
  [0xed] = 0x03C6,
  [0xee] = 0x03B5,
  [0xef] = 0x2229,
  [0xf0] = 0x2261,
  [0xf1] = 0x00B1,
  [0xf2] = 0x2265,
  [0xf3] = 0x2264,
  [0xf4] = 0x2320,
  [0xf5] = 0x2321,
  [0xf6] = 0x00F7,
  [0xf7] = 0x2248,
  [0xf8] = 0x00B0,
  [0xf9] = 0x2219,
  [0xfa] = 0x00B7,
  [0xfb] = 0x221A,
  [0xfc] = 0x207F,
  [0xfd] = 0x00B2,
  [0xfe] = 0x25A0,
  [0xff] = 0x00A0,
};
static const struct gap from_idx[] = {
  { .start = 0x0000, .end = 0x007f, .idx =     0 },
  { .start = 0x00a0, .end = 0x00a2, .idx =   -32 },
  { .start = 0x00ab, .end = 0x00bd, .idx =   -40 },
  { .start = 0x00c4, .end = 0x00c4, .idx =   -46 },
  { .start = 0x00d5, .end = 0x00e4, .idx =   -62 },
  { .start = 0x00f5, .end = 0x0105, .idx =   -78 },
  { .start = 0x010c, .end = 0x0119, .idx =   -84 },
  { .start = 0x0122, .end = 0x0123, .idx =   -92 },
  { .start = 0x012a, .end = 0x012f, .idx =   -98 },
  { .start = 0x0136, .end = 0x013c, .idx =  -104 },
  { .start = 0x0145, .end = 0x0146, .idx =  -112 },
  { .start = 0x0160, .end = 0x0161, .idx =  -137 },
  { .start = 0x016a, .end = 0x016b, .idx =  -145 },
  { .start = 0x0172, .end = 0x0173, .idx =  -151 },
  { .start = 0x017d, .end = 0x017e, .idx =  -160 },
  { .start = 0x0393, .end = 0x0398, .idx =  -692 },
  { .start = 0x03a3, .end = 0x03a9, .idx =  -702 },
  { .start = 0x03b1, .end = 0x03b5, .idx =  -709 },
  { .start = 0x03c0, .end = 0x03c6, .idx =  -719 },
  { .start = 0x201e, .end = 0x201e, .idx = -7974 },
  { .start = 0x207f, .end = 0x207f, .idx = -8070 },
  { .start = 0x2219, .end = 0x221e, .idx = -8479 },
  { .start = 0x2229, .end = 0x2229, .idx = -8489 },
  { .start = 0x2248, .end = 0x2248, .idx = -8519 },
  { .start = 0x2261, .end = 0x2265, .idx = -8543 },
  { .start = 0x2310, .end = 0x2310, .idx = -8713 },
  { .start = 0x2320, .end = 0x2321, .idx = -8728 },
  { .start = 0x2500, .end = 0x2502, .idx = -9206 },
  { .start = 0x250c, .end = 0x251c, .idx = -9215 },
  { .start = 0x2524, .end = 0x2524, .idx = -9222 },
  { .start = 0x252c, .end = 0x252c, .idx = -9229 },
  { .start = 0x2534, .end = 0x2534, .idx = -9236 },
  { .start = 0x253c, .end = 0x253c, .idx = -9243 },
  { .start = 0x2550, .end = 0x256c, .idx = -9262 },
  { .start = 0x2580, .end = 0x2593, .idx = -9281 },
  { .start = 0x25a0, .end = 0x25a0, .idx = -9293 },
  { .start = 0xffff, .end = 0xffff, .idx =     0 }
};
static const char from_ucs4[] = {

  '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
  '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
  '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
  '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
  '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',
  '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
  '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
  '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',
  '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
  '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f',
  '\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57',
  '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
  '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67',
  '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',
  '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
  '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',
  '\xff', '\x00', '\x9b', '\xae', '\xaa', '\x00', '\x00', '\x00',
  '\xf8', '\xf1', '\xfd', '\x00', '\x00', '\xe6', '\x00', '\xfa',
  '\x00', '\x00', '\x00', '\xaf', '\xac', '\xab', '\x8e', '\x95',
  '\x99', '\x00', '\x00', '\x00', '\x00', '\x00', '\x9a', '\x00',
  '\x00', '\xe1', '\x00', '\x00', '\x00', '\x00', '\x84', '\x93',
  '\x94', '\xf6', '\x00', '\x00', '\x00', '\x00', '\x81', '\x00',
  '\x00', '\x00', '\xa0', '\x83', '\x00', '\x00', '\x8f', '\x85',
  '\x80', '\x87', '\x00', '\x00', '\x00', '\x00', '\x89', '\x88',
  '\x00', '\x00', '\x90', '\x82', '\x8b', '\x8a', '\xa8', '\x98',
  '\xa1', '\x8c', '\x00', '\x00', '\xad', '\x8d', '\xa3', '\xa2',
  '\x00', '\x00', '\x00', '\x9c', '\x86', '\xa5', '\xa4', '\x9f',
  '\x9e', '\xa6', '\x96', '\xa7', '\x97', '\x92', '\x91', '\xe2',
  '\x00', '\x00', '\x00', '\x00', '\xe9', '\xe4', '\x00', '\x00',
  '\xe8', '\x00', '\x00', '\xea', '\xe0', '\x00', '\x00', '\xeb',
  '\xee', '\xe3', '\x00', '\x00', '\xe5', '\xe7', '\x00', '\xed',
  '\x9d', '\xfc', '\xf9', '\xfb', '\x00', '\x00', '\x00', '\xec',
  '\xef', '\xf7', '\xf0', '\x00', '\x00', '\xf3', '\xf2', '\xa9',
  '\xf4', '\xf5', '\xc4', '\x00', '\xb3', '\xda', '\x00', '\x00',
  '\x00', '\xbf', '\x00', '\x00', '\x00', '\xc0', '\x00', '\x00',
  '\x00', '\xd9', '\x00', '\x00', '\x00', '\xc3', '\xb4', '\xc2',
  '\xc1', '\xc5', '\xcd', '\xba', '\xd5', '\xd6', '\xc9', '\xb8',
  '\xb7', '\xbb', '\xd4', '\xd3', '\xc8', '\xbe', '\xbd', '\xbc',
  '\xc6', '\xc7', '\xcc', '\xb5', '\xb6', '\xb9', '\xd1', '\xd2',
  '\xcb', '\xcf', '\xd0', '\xca', '\xd8', '\xd7', '\xce', '\xdf',
  '\x00', '\x00', '\x00', '\xdc', '\x00', '\x00', '\x00', '\xdb',
  '\x00', '\x00', '\x00', '\xdd', '\x00', '\x00', '\x00', '\xde',
  '\xb0', '\xb1', '\xb2', '\xfe',
};
