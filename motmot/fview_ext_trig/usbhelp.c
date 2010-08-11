#include "usbhelp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void xx(struct libusb_transfer* transfer_p) {
  struct libusb_iso_packet_descriptor *val;
  int i,lengthval;

  printf("transfer_p at %p\n",transfer_p);

  printf("  &(transfer_p->buffer): %p\n",&(transfer_p->buffer) );

  printf("  &(transfer_p->num_iso_packets): %p\n",&(transfer_p->num_iso_packets) );
  printf("  transfer_p->num_iso_packets: %d\n",transfer_p->num_iso_packets );

  printf("  &(transfer_p->iso_packet_desc): %p\n",&(transfer_p->iso_packet_desc) );
  val = transfer_p->iso_packet_desc;
  //printf("  transfer_p->iso_packet_desc: 0x%0x\n", (long)(transfer_p->iso_packet_desc) );
  printf("  &val = %p\n",&val);
  printf("   val = %p\n",val);
  //val = transfer_p->iso_packet_desc;

  for (i=0; i<transfer_p->num_iso_packets; i++) {
    lengthval = transfer_p->iso_packet_desc[i].length;
    printf("    transfer_p->iso_packet_desc[%d].length = %d\n",i,lengthval);
  }


}


void usbhelp_libusb_set_iso_packet_lengths(struct libusb_transfer *transfer_p,
                                           unsigned int length) {
  libusb_set_iso_packet_lengths(transfer_p, length);
}

int usbhelp_get_actual_iso_size( struct libusb_transfer* transfer_p,
                                 int* cumsize ){
  int err;
  int i;
  int tmpcum, lengthval;

  tmpcum = 0;
  *cumsize = 0;

  if (transfer_p->status!=LIBUSB_TRANSFER_COMPLETED) {
    err = -1;
    return err;
  }

  for (i=0; i<transfer_p->num_iso_packets; i++) {
    lengthval = transfer_p->iso_packet_desc[i].actual_length;
    tmpcum += lengthval;
  }

  *cumsize = tmpcum;
  err = 0;

  return err;
}

int usbhelp_copy_transfer_packets( struct libusb_transfer*transfer_p, char *buf, int sz ) {
  int err;
  int i,j;
  size_t lengthval;
  size_t cumsz;
  unsigned char* srcptr, *s2;

  cumsz = 0;

  if (transfer_p->status!=LIBUSB_TRANSFER_COMPLETED) {
    err = -1;
    return err;
  }

  for (i=0; i<transfer_p->num_iso_packets; i++) {

    lengthval = transfer_p->iso_packet_desc[i].actual_length;
    if ((cumsz+lengthval) > sz ) {
      err = -2;
      return err;
    }

    srcptr = libusb_get_iso_packet_buffer_simple(transfer_p, i);
    if (srcptr==NULL) {
      err = -3;
      return err;
    }

    memcpy( buf, srcptr, lengthval);

    for (j=0; j<lengthval; j++) {
      s2 = srcptr + j;
      printf("                                                      c:0x%02hhx\n",*s2);
    }

    cumsz += lengthval;
    buf += lengthval;
  }

  err = 0;

  return err;
}
