#ifndef USB_H
#define USB_H

#include <stdint.h>

// USB Request Types
#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_REQ_SET_ADDRESS    0x05
#define USB_REQ_SET_CONFIGURATION 0x09

// Descriptor Types
#define USB_DESC_DEVICE        0x01
#define USB_DESC_CONFIGURATION 0x02
#define USB_DESC_INTERFACE     0x04
#define USB_DESC_ENDPOINT      0x05

// USB Device Request
struct usb_device_request {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} __attribute__((packed));

// Device Descriptor
struct usb_device_descriptor {
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t usb_version;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    uint8_t max_packet_size;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_version;
    uint8_t manufacturer_string;
    uint8_t product_string;
    uint8_t serial_string;
    uint8_t num_configurations;
} __attribute__((packed));

// Configuration Descriptor
struct usb_config_descriptor {
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t total_length;
    uint8_t num_interfaces;
    uint8_t configuration_value;
    uint8_t configuration_string;
    uint8_t attributes;
    uint8_t max_power;
} __attribute__((packed));

// USB declarations
void usb_init();
void usb_poll();

#endif
