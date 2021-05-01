#include "memory_manager.hpp"

#include <cstddef>

BitmapMemoryManager::BitmapMemoryManager()
    : alloc_map_{}, range_begin_{FrameId{0}}, range_end_{FrameId{kFrameCount}} {}

WithError<FrameId> BitmapMemoryManager::Allocate(size_t num_frames) {
  size_t start_frame_id = range_begin_.Id();
  while (true) {
    size_t i = 0;
    for (; i < num_frames; ++i) {
      if (start_frame_id + i >= range_end_.Id()) {
        return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
      }
      if (GetBit(FrameId{start_frame_id + i})) {
        // "start_frame_id + i" にあるフレームは割り当て済み
        break;
      }
    }
    if (i == num_frames) {
      // num_frames 分の空きが見つかった
      MarkAllocated(FrameId{start_frame_id}, num_frames);
      return {
          FrameId{start_frame_id},
          MAKE_ERROR(Error::kSuccess),
      };
    }
    // 次のフレームから再検索
    start_frame_id += i + 1;
  }
}

Error BitmapMemoryManager::Free(FrameId start_frame, size_t num_frames) {
  for (size_t i = 0; i < num_frames; ++i) {
    SetBit(FrameId{start_frame.Id() + i}, false);
  }
  return MAKE_ERROR(Error::kSuccess);
}

void BitmapMemoryManager::MarkAllocated(FrameId start_frame, size_t num_frames) {
  for (size_t i = 0; i < num_frames; ++i) {
    SetBit(FrameId{start_frame.Id() + i}, true);
  }
}

void BitmapMemoryManager::SetMemoryRange(FrameId range_begin, FrameId range_end) {
  range_begin_ = range_begin;
  range_end_ = range_end;
}

bool BitmapMemoryManager::GetBit(FrameId frame) const {
  auto line_index = frame.Id() / kBitsPerMapLine;
  auto bit_index = frame.Id() % kBitsPerMapLine;

  return (alloc_map_[line_index] & (static_cast<MapLineType>(1) << bit_index)) != 0;
}

void BitmapMemoryManager::SetBit(FrameId frame, bool allocated) {
  auto line_index = frame.Id() / kBitsPerMapLine;
  auto bit_index = frame.Id() % kBitsPerMapLine;

  if (allocated) {
    alloc_map_[line_index] |= (static_cast<MapLineType>(1) << bit_index);
  } else {
    alloc_map_[line_index] &= ~(static_cast<MapLineType>(1) << bit_index);
  }
}

extern "C" char *program_break, *program_break_end;

Error InitializeHeap(BitmapMemoryManager &memory_manager) {
  const int kHeapFrames = 64 * 512;
  const auto heap_start = memory_manager.Allocate(kHeapFrames);
  if (heap_start.error) {
    return heap_start.error;
  }

  program_break = reinterpret_cast<char *>(heap_start.value.Id() * kBytesPerFrame);
  program_break_end = program_break + kHeapFrames * kBytesPerFrame;
  return MAKE_ERROR(Error::kSuccess);
}
