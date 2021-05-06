#include "../syscall.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" void main(int argc, char **argv) {
  auto [layer_id, err_openwin] = SyscallOpenWindow(200, 100, 10, 10, u8"java.com");
  if (err_openwin) {
    exit(err_openwin);
  }

  SyscallWinWriteString(layer_id, 100 - 8 * 13 / 2, 24, 0x646464, u8"あなたとJAVA,");
  SyscallWinWriteString(layer_id, 100 - 8 * 16 / 2, 40, 0x646464, u8"今すぐダウンロー");
  SyscallWinWriteString(layer_id, 100 - 8 * 2 / 2, 56, 0x646464, u8"ド");

  int width = 8 * 12;
  SyscallWinFillRectangle(layer_id, 100 - width / 2, 72, width, 16, 0xff0000);
  SyscallWinWriteString(layer_id, 100 - width / 2, 72, 0xffffff, u8"無料Javaのダ");

  AppEvent events[1];
  while (true) {
    auto [n, err] = SyscallReadEvent(events, 1);
    if (err) {
      printf("ReadEvent failed: %s\n", strerror(err));
      break;
    }
    if (events[0].type == AppEvent::kQuit) {
      break;
    } else {
      printf("unknown event: type = %d\n", events[0].type);
    }
  }
  SyscallCloseWindow(layer_id);
  exit(0);
}
