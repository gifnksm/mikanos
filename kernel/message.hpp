#pragma once

struct Message {
  enum Type {
    kInterruptXhci,
    kTimerTimeout,
  } type;

  union {
    struct {
      unsigned long timeout;
      int value;
    } timer;
  } arg;
};
