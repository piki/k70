#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os.h"

struct onekey {
  int r, g, b;
};

typedef struct onekey keymap[144];

/*
static unsigned char packets[5][64] = {
  { 0x7f, 0x01, 0x3c, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77 },
  { 0x7f, 0x02, 0x3c, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x7f, 0x03, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x7f, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x07, 0x27, 0x00, 0x00, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};
*/

void fill_packets(keymap *map, unsigned char packets[5][64]) {
  int i;
  for (i=0; i<5; i++)
    bzero(packets[i], 64);

  memcpy(packets[0], "\x7f\x01\x3c\0", 4);
  memcpy(packets[1], "\x7f\x02\x3c\0", 4);
  memcpy(packets[2], "\x7f\x03\x3c\0", 4);
  memcpy(packets[3], "\x7f\x04\x24\0", 4);
  memcpy(packets[4], "\x07\x27\0\0\xd8", 5);

  for (i=0; i<72; i++) {
    int ri = 0*72 + i;
    int gi = 1*72 + i;
    int bi = 2*72 + i;
    packets[ri/60][4+(ri%60)] = ((7-(*map)[i*2].r)<<4) + 7-(*map)[i*2+1].r;
    packets[gi/60][4+(gi%60)] = ((7-(*map)[i*2].g)<<4) + 7-(*map)[i*2+1].g;
    packets[bi/60][4+(bi%60)] = ((7-(*map)[i*2].b)<<4) + 7-(*map)[i*2+1].b;
  }
}

// 0: backtick/tilde
// 1: escape
// 2: caps lock
// 3: tab
// 4: lctrl
// 5: lshift
// 6: equals/plus
// 7: F12
// 8: KP_7
// 9: windows lock indicator
// 10: 
// 11: 
// 12: 1
// 13: F1
// 14: A
// 15: Q
// 16: lmeta
// 17: 
// 18: 
// 19: prtscn/sysrq
// 20: KP_8
// 21: mute
// 22: 
// 23: 
// 24: 2
// 25: F2
// 26: S
// 27: W
// 28: lalt
// 29: Z
// 30: backspace
// 31: scroll lock
// 32: KP_9
// 33: media_stop
// 34: 
// 35: 
// 36: 3
// 37: F3
// 38: D
// 39: E
// 40: 
// 41: X
// 42: Delete
// 43: pause/break
// 44: 
// 45: media_rewind
// 46: 
// 47: 
// 48: 4
// 49: F4
// 50: F
// 51: R
// 52: space
// 53: C
// 54: End
// 55: Insert
// 56: KP_4
// 57: media_play/pause
// 58: 
// 59: 
// 60: 5
// 61: F5
// 62: G
// 63: T
// 64: 
// 65: V
// 66: pgdown
// 67: Home
// 68: KP_5
// 69: media_ffwd
// 70: 
// 71: 
// 72: 6
// 73: F6
// 74: H
// 75: Y
// 76: 
// 77: B
// 78: rshift
// 79: Page Up
// 80: KP_6
// 81: Num Lock
// 82: 
// 83: 
// 84: 7
// 85: F7
// 86: J
// 87: U
// 88: ralt
// 89: N
// 90: rctrl
// 91: ] }
// 92: KP_1
// 93: KP_slash
// 94: 
// 95: 
// 96: 8
// 97: F8
// 98: K
// 99: I
// 100: rmeta
// 101: M
// 102: arrow_up
// 103: pipe/backslash
// 104: KP_2
// 105: KP_star
// 106: 
// 107: 
// 108: 9
// 109: F9
// 110: L
// 111: O
// 112: menu
// 113: <
// 114: arrow_left
// 115: 
// 116: KP_3
// 117: KP_minus
// 118: 
// 119: 
// 120: 0
// 121: F10
// 122: : ;
// 123: P
// 124: 
// 125: >
// 126: arrow_down
// 127: enter
// 128: KP_0
// 129: KP_plus
// 130: 
// 131: 
// 132: - _
// 133: F11
// 134: ' "
// 135: [ {
// 136: brightness
// 137: / ?
// 138: arrow_right
// 139: 
// 140: KP_period / delete
// 141: KP_enter
// 142: 
// 143: 

static const char *rows[7] = {
  "                                                   136 9          21",
  "1   13 25 37 49   61 73 85 97   109 121 133 7      19  31 43   33 45  57  69",
  "0   12 24 36 48 60 72 84 96 108 120   132 6 30     55  67 79   81 93  105 117",
  "3   15 27 39 51 63 75 87 99 111 123   135 91 103   42  54 66   8  20  32  129",
  "2   14 26 38 50 62 74 86 98 110    122 134 127                 56 68  80",
  "5      29 41 53 65 77 89 101   113 125 137 78         102      92 104 116 141",
  "4 16 28           52           88 100 112 90      114 126 138  128    140",
};

static const struct { const char *name, *values; } keysets[] = {
  { "function keys", "13 25 37 49   61 73 85 97   109 121 133 7" },
  { "numbers", "12 24 36 48 60 72 84 96 108 120" },
  { "letters", "15 27 39 51 63 75 87 99 111 123   14 26 38 50 62 74 86 98 110    29 41 53 65 77 89 101" },
  { "arrow keys", "102 114 126 138" },
  { "keypad", "81 93 105 117   8 20 32 129   56 68 80   92 104 116 141   128 140" },
};

void set_keys(const char *keylist, struct onekey clr, keymap *map) {
  char *dup = strdup(keylist);
  for (const char *p=strtok(dup, " "); p; p=strtok(NULL, " ")) {
    int code = atoi(p);
    (*map)[code] = clr;
  }
  free(dup);
}

void make_rainbow(keymap *map) {
  bzero(map, sizeof(*map));
  struct onekey row_colors[7] = {
    { .r=7, .g=7, .b=7 },
    { .r=7, .g=0, .b=0 },
    { .r=7, .g=3, .b=0 },
    { .r=7, .g=7, .b=0 },
    { .r=0, .g=7, .b=0 },
    { .r=0, .g=0, .b=7 },
    { .r=7, .g=0, .b=7 },
  };
  for (int i=0; i<7; i++) {
    set_keys(rows[i], row_colors[i], map);
  }
}

void make_diagonal_rainbow(keymap *map) {
  bzero(map, sizeof(*map));
  struct onekey white = {
    .r=7, .g=7, .b=7
  };
  struct onekey row_colors[] = {
    { .r=7, .g=0, .b=0 },  // red
    { .r=7, .g=1, .b=0 },
    { .r=7, .g=2, .b=0 },
    { .r=7, .g=3, .b=0 },  // orange
    { .r=7, .g=4, .b=0 },
    { .r=7, .g=5, .b=0 },
    { .r=7, .g=6, .b=0 },
    { .r=7, .g=7, .b=0 },  // yellow
    { .r=5, .g=7, .b=0 },
    { .r=3, .g=7, .b=0 },
    { .r=1, .g=7, .b=0 },
    { .r=0, .g=7, .b=0 },  // green
    { .r=0, .g=5, .b=1 },
    { .r=0, .g=3, .b=3 },
    { .r=0, .g=1, .b=5 },
    { .r=0, .g=0, .b=7 },  // blue
    { .r=1, .g=0, .b=7 },
    { .r=3, .g=0, .b=7 },
    { .r=5, .g=0, .b=7 },
    { .r=7, .g=0, .b=7 },  // purple
  };
  const int ncolors = sizeof(row_colors)/sizeof(row_colors[0]);
  set_keys(rows[0], white, map);  // media keys <- white
  int max_score = 0;
  for (int i=1; i<7; i++) {
    for (int j=0; rows[i][j]; ) {
      while (isspace(rows[i][j])) j++;
      int score = 3*i + j;
      if (score > max_score) max_score = score;
      while (rows[i][j] && !isspace(rows[i][j])) j++;
    }
  }

  for (int i=1; i<7; i++) {
    for (int j=0; rows[i][j]; ) {
      while (isspace(rows[i][j])) j++;
      int keyno = atoi(&rows[i][j]);
      int score = 3*i + j;
      int color = score*ncolors/(max_score+1);
      (*map)[keyno] = row_colors[color];
      while (rows[i][j] && !isspace(rows[i][j])) j++;
    }
  }
}

void make_sunset(keymap *map) {
  bzero(map, sizeof(*map));
  struct onekey blue = { .r=0, .g=0, .b=5 };
  struct onekey red = { .r=6, .g=0, .b=0 };
  struct onekey orange = { .r=6, .g=4, .b=0 };
  struct onekey yellow = { .r=6, .g=6, .b=0 };
  for (int i=0; i<144; i++) {
    (*map)[i] = blue;
  }

  set_keys(keysets[1].values, orange, map);
  set_keys(keysets[2].values, red, map);
  set_keys(keysets[3].values, yellow, map);
  set_keys(keysets[4].values, orange, map);
}

void usage(const char *prog) {
  printf("Usage:\n  %s {rainbow|drainbow|sunset}\n", prog);
  exit(1);
}

int main(int argc, char **argv) {
  keymap map;
  bzero(&map, sizeof(map));
  if (argc != 2)
    usage(argv[0]);

  if (!strcmp(argv[1], "rainbow"))
    make_rainbow(&map);
  if (!strcmp(argv[1], "drainbow"))
    make_diagonal_rainbow(&map);
  else if (!strcmp(argv[1], "sunset"))
    make_sunset(&map);
  else
    usage(argv[0]);

  struct kbd *kbd = usbopen();
  if (!kbd) {
    usbclose(NULL);
    return 1;
  }

  unsigned char packets[5][64];

  fill_packets(&map, packets);

  usbsend(kbd, packets[0]);
  usbsend(kbd, packets[1]);
  usbsend(kbd, packets[2]);
  usbsend(kbd, packets[3]);
  usbsend(kbd, packets[4]);

  usbclose(kbd);

  return 0;
}
