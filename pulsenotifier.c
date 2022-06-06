// gcc -o pulsenotifier -I/usr/include/pulse/ pulsenotifier.c -lpulse -lhidapi-libusb

// USBRelay2

#include <stddef.h>
#include <hidapi/hidapi.h>
#include <stdio.h>

void usbrelay2_set_relay(char state) {
  hid_init();
  unsigned short vendor_id = 0x16c0;
  unsigned short product_id = 0x05df;
  const wchar_t *serial_number = NULL; // NOTE: USBRelay2 do not make use of this kind of serial number
  hid_device * dev = hid_open(vendor_id, product_id, serial_number); // connect to any USBRelay2 (regardless of serial number)
  if (dev == NULL) {
    printf("%ls\n", hid_error(NULL));
    //printf("Error: No USBRelay2 can be opened. Check connections and permissions.\n");
  } else {
    char command[] = "\xFD\x01\x00\x00\x00\x00\x00"; // c string adds a null-byte (see below)
    const size_t command_length = sizeof(command)/sizeof(*command); // must be 8 (see above)
    // command[0]: FF means "on", FD means "off"
    // command[1]: is relay index (starting at 1)
    // others: unknown
    command[0] += state;
    hid_write(dev, command, command_length);
    hid_close(dev);
  }
  hid_exit();
}

// pulseaudio

#include <pulseaudio.h>
#include <stdio.h>
#include <string.h>

struct pulsenotifier_data_t {
  pa_mainloop * mainloop;
  const char * source_name;
  void (*source_state_changed_cb)(char);
};
typedef struct pulsenotifier_data_t pulsenotifier_data_t;

static void source_info_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
  printf("source_info_cb\n");
  if (i == NULL) {
    return;
  }
  if (eol > 0) {
    return;
  }
  int running = i->state == PA_SOURCE_RUNNING;
  int relevant = 1;
  printf("  name: %s\n", i->name);
  printf("  running: %d\n", running);
  if (userdata) {
    pulsenotifier_data_t * pulsenotifier_data = (pulsenotifier_data_t *)userdata;
    if (pulsenotifier_data->source_name) {
      relevant = strcmp(pulsenotifier_data->source_name, i->name) == 0;
    }
    if (relevant && pulsenotifier_data->source_state_changed_cb) {
      pulsenotifier_data->source_state_changed_cb(running);
    }
  }
}

static void context_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
  printf("context_subscribe_cb 0x%x %u\n", t, idx);
  if (
    ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SOURCE) &&
    ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE)
  ) {
    pa_context_get_source_info_by_index(c, idx, source_info_cb, userdata);
  }
}

static void context_success_cb(pa_context *c, int success, void *userdata) {
}

static void pa_context_notify_cb(pa_context *c, void *userdata) {
  pulsenotifier_data_t * pulsenotifier_data = (pulsenotifier_data_t *)userdata;
  switch (pa_context_get_state(c)) {
    case PA_CONTEXT_READY:
      pa_context_set_subscribe_callback(c, context_subscribe_cb, userdata);
      pa_subscription_mask_t subscription_mask = PA_SUBSCRIPTION_MASK_ALL;
      subscription_mask = PA_SUBSCRIPTION_MASK_SOURCE;
      pa_context_subscribe(c, subscription_mask, context_success_cb, userdata);
      pa_context_get_source_info_list(c, source_info_cb, userdata);
      break;

    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
      // terminate on any error
      pa_mainloop_quit(pulsenotifier_data->mainloop, 1);
      break;

    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_SETTING_NAME:
    case PA_CONTEXT_AUTHORIZING:
      // these are not relevant to my operation
      break;

    default:
      printf("Unknown pulseaudio context state\n");
      pa_mainloop_quit(pulsenotifier_data->mainloop, 2);
      break;
    }
}

int main(int argc, char **argv) {
  pulsenotifier_data_t pulsenotifier_data;
  if (argc > 1) {
    pulsenotifier_data.source_name = argv[1];
  }
  pulsenotifier_data.source_state_changed_cb = usbrelay2_set_relay;
  pulsenotifier_data.mainloop = pa_mainloop_new();
  pa_context * c = pa_context_new(pa_mainloop_get_api(pulsenotifier_data.mainloop), "pulsenotifier");
  pa_context_set_state_callback(c, pa_context_notify_cb, &pulsenotifier_data);
  int err = pa_context_connect(c, NULL, PA_CONTEXT_NOFAIL, NULL);
  if (err < 0) {
    printf("pa_context_connect() failed: %s\n", pa_strerror(err));
    return err;
  } else {
    int retval;
    pa_mainloop_run(pulsenotifier_data.mainloop, &retval);
    return retval;
  }
}
