// Kode kernel
// PENTING : FUNGSI PERTAMA YANG DIDEFINISIKAN ADALAH main(),
// ek spesifikasi untuk informasi lebih lanjut

// TODO : Tambahkan implementasi kode C

#include "header/kernel.h"

int main() {
    char buf[128];
    //call the bootloader
    fillKernelMap();
    clearScreen();
    makeInterrupt21();
    interrupt(0x10, 0x0200, 0, 0, 0x0307);
    printString("       ____  _   _ ____  _                 _            ___  ____\r\n");
    interrupt(0x10, 0x0200, 0, 0, 0x0407);
    printString(" _   _/ ___|| | | / ___|| |__  _   _ _ __ | |_ _   _   / _ \\/ ___|\r\n");
    interrupt(0x10, 0x0200, 0, 0, 0x0507);
    printString("| | | \\___ \\| | | \\___ \\| '_ \\| | | | '_ \\| __| | | | | | | \\___ \\\r\n");
    interrupt(0x10, 0x0200, 0, 0, 0x0607);
    printString("| |_| |___) | |_| |___) | |_) | |_| | | | | |_| |_| | | |_| |___) |\r\n");
    interrupt(0x10, 0x0200, 0, 0, 0x0707);
    printString(" \\__,_|____/ \\___/|____/|_.__/ \\__,_|_| |_|\\__|\\__,_|  \\___/|____/\r");
    interrupt(0x10, 0x0200, 0, 0, 0x0928 - 0x0C);
    printString("Welcome to uSUSbuntu OS\r\n");
    interrupt(0x10, 0x0200, 0, 0, 0x0A28 - 0x12);
    printString("Press enter any key to get started\r\n\n");
    //wait for user input
    while (true) {
      interrupt(0x21, 1, buf, 0, 0);
      interrupt(0x21, 0, buf, 0, 0);
      interrupt(0x21, 0, "\r\n", 0, 0);
    }
}

void handleInterrupt21(int AX, int BX, int CX, int DX) {
    switch (AX) {
        case 0x0:
            printString(BX);
            break;
        case 0x1:
            readString(BX);
            break;
        default:
            printString("Invalid Interrupt");
    }
}

//0xB8000 <- charater in top left (x = 0, y = 0)
//0xB000 <- start segment
//0x8000 + (80*y + x) * 2 <- y
//karna ukurannya 80x25 berarti setiap huruf pindah 80

void printString(char *string) {
    int i = 0;
    char c = string[i];
    byte color = 0xD;
    while (c != '\0') {
        interrupt(0x10, 0x0E00 + c, 0, 0, 0);
        i++;
        c = string[i];
    }
}

void readString(char *string) {
    bool check = true;
    char c = '\0';
    int i = 0;
    int j = 0;
    while (check) {
        //read character <- user input
        c = interrupt(0x16, 0x0, 0, 0, 0);
        if (c == '\r' || c == '\n') { //carriage and newline
            string[i++] = '\0';
            check = false;
            printString("\r\n");

        } else if (c == '\b') { //backspace
            interrupt(0x10, 0x0E00 + '\b', 0, 0, 0);
            interrupt(0x10, 0x0E00 + ' ', 0, 0, 0);
            interrupt(0x10, 0x0E00 + '\b', 0, 0, 0);
            string[i] = '\0';
            
            if (i > 0){
              i--;
            }

        }else if(c == '\t'){
          for(j = 0; j < 4; j++){
            string[i++] = ' ';
            interrupt(0x10, 0x0E00 + ' ', 0, 0, 0);
          }
        } 
        else { //insert to buffer but don't exceed the boundary
            string[i] = c;
            interrupt(0x10, 0x0E00 + c, 0, 0, 0);

            if(i < 127){
              i++;
            }
        }
    }
}

void clearScreen() {
  interrupt(0x10, 0x0200, 0, 0, 0);
  interrupt(0x10, 0x0600, 0x0f00, 0, 0x184f);
}

void writeSector(byte *buffer, int sector_number) {
  interrupt(0x13, 0x0201, buffer, div(sector_number, 36) << 8 + mod(sector_number, 18) + 1, mod(div(sector_number, 18), 2) << 8);
}

void readSector(byte *buffer, int sector_number) {
  interrupt(0x13, 0x0301, buffer, div(sector_number, 36) << 8 + mod(sector_number, 18) + 1, mod(div(sector_number, 18), 2) << 8);
}

void fillKernelMap(){
  struct map_filesystem map_fs_buffer;
  int i;
  // Load filesystem map
  readSector(&map_fs_buffer, FS_MAP_SECTOR_NUMBER);
  for(i = 0; i < 512; i++) {
    if(i < 16 || i > 256){
      map_fs_buffer.is_filled[i] = true;
    } else {
      map_fs_buffer.is_filled[i] = false;
    }
  }
  writeSector(&map_fs_buffer, FS_MAP_SECTOR_NUMBER); 
}

void read(struct file_metadata *metadata, enum fs_retcode *return_code) {
  struct node_filesystem   node_fs_buffer;
  struct sector_filesystem sector_fs_buffer;
  int i = 0;
  int j = 0;
  bool found = false;
  byte file[16];
  // Tambahkan tipe data yang dibutuhkan
  // Masukkan filesystem dari storage ke memori buffer

  // 1. Cari node dengan nama dan lokasi yang sama pada filesystem.
  //    Jika ditemukan node yang cocok, lanjutkan ke langkah ke-2.
  //    Jika tidak ditemukan kecocokan, tuliskan retcode FS_R_NODE_NOT_FOUND
  //    dan keluar. 

  readSector(&node_fs_buffer, FS_NODE_SECTOR_NUMBER);
  readSector(&node_fs_buffer.nodes[32], FS_NODE_SECTOR_NUMBER + 1);
  readSector(&sector_fs_buffer, FS_SECTOR_SECTOR_NUMBER);

  while (i < 64 && !found) {
    if (strcmp(node_fs_buffer.nodes[i].name, metadata->node_name) && 
    metadata->parent_index == node_fs_buffer.nodes[i].parent_node_index){
      found = true;
    }
    i++;
  }

  if (!found){
    return_code = FS_R_NODE_NOT_FOUND;
    return;
  } 
  
  // 2. Cek tipe node yang ditemukan
  //    Jika tipe node adalah file, lakukan proses pembacaan.
  //    Jika tipe node adalah folder, tuliskan retcode FS_R_TYPE_IS_FOLDER
  //    dan keluar.

  if (node_fs_buffer.nodes[i].sector_entry_index == FS_NODE_S_IDX_FOLDER) {
    return_code = FS_R_TYPE_IS_FOLDER;
    return;
  }

  // Pembacaan
  // 1. memcpy() entry sector sesuai dengan byte S
  // 2. Lakukan iterasi proses berikut, i = 0..15
  // 3. Baca byte entry sector untuk mendapatkan sector number partisi file
  // 4. Jika byte bernilai 0, selesaikan iterasi
  // 5. Jika byte valid, lakukan readSector() 
  //    dan masukkan kedalam buffer yang disediakan pada metadata
  // 6. Lompat ke iterasi selanjutnya hingga iterasi selesai
  // 7. Tulis retcode FS_SUCCESS dan ganti filesize 
  // pada akhir proses pembacaan.
  clear(metadata->buffer, metadata->filesize);
  memcpy(&file, &sector_fs_buffer.sector_list[node_fs_buffer.nodes[i].sector_entry_index], 16);
  for(j = 0; j < 16; j++){
    if(file[j]){
      readSector(metadata->buffer[j], file[j]);
      continue;
    }
    break;
  }

  metadata->filesize = j;
  return_code = FS_SUCCESS;
}
