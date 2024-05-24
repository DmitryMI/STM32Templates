#if 0

#ifndef UIP_DEBUG_H
#define UIP_DEBUG_H
extern "C" {
  #include "utility/uip.h"
}

class UIPDebug {

public:

  static void uip_debug_printconns();
  static bool uip_debug_printcon(struct uip_conn *lhs,struct uip_conn *rhs);
  static void uip_debug_printbytes(const uint8_t *data, uint8_t len);

};


#endif

#endif
