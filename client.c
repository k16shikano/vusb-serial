#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include <libusb-1.0/libusb.h>
#include <unistd.h>

libusb_device **list;
libusb_device_handle *handle;
struct libusb_device_descriptor desc;
unsigned char buffer[512];
unsigned char buffer2[512];
int cnt, i;
struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI };

void scan_text ();
void receive_text ();

int main() {
  libusb_init(NULL);
  cnt = libusb_get_device_list(NULL, &list);
  
  for (i = 0; i < cnt; i++) {
    libusb_device *device = list[i];
    libusb_get_device_descriptor(device, &desc);
    if (libusb_open(device, &handle) == 0) {
      
      libusb_get_string_descriptor_ascii(handle, desc.iProduct, buffer, sizeof(buffer));
        
      if (strcmp("Template", (char const *)buffer) == 0) {
        while (1) {
          
          if (poll(&mypoll, 1, 1000)) {
            unsigned char *buf = malloc(128);
            scan_text(handle, buf);
            //            printf("Scan string - %s\n", buf);
            free(buf);
          } else {
            receive_text(handle);
            //            usleep(2000000);
          }
        }
        libusb_close(handle);
      }
    }
  }
  libusb_free_device_list(list, 1);
  libusb_exit(NULL);  
  return 0;
}

void scan_text (libusb_device_handle* handle, unsigned char *buf) {
  fgets((char*)buf, 128, stdin);
  printf("input %ld\n", strlen((char*)buf));
  
  libusb_control_transfer(handle, 
                          LIBUSB_REQUEST_TYPE_VENDOR | \
                          LIBUSB_RECIPIENT_DEVICE | \
                          LIBUSB_ENDPOINT_OUT, 
                          0, 0, 0, buf, strlen((char*)buf)+1, 5000);
}
 
void receive_text (libusb_device_handle* handle) {
  unsigned char *buf = malloc(16);
  int r = libusb_control_transfer(handle, 
                                  LIBUSB_REQUEST_TYPE_VENDOR |  \
                                  LIBUSB_RECIPIENT_DEVICE |     \
                                  LIBUSB_ENDPOINT_IN, 
                                  0, 0, 0, buf, 16, 5000);
  if (r > 0)
    printf("%s\n", buf);
  //printf("%d\n", r);
}
