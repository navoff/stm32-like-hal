#pragma once

/**
 * @file nmea_core.h
 * @brief NMEA sentence parser: GGA, RMC and GSA decoding, checksum and message framing.
 */

#include <stdint.h>

#include "config.h"

#include "utils/errors.h"

/**
 * @brief Sizes of the arrays allocated for data
 * @addtogroup NMEA_PACKET_MAX_SIZES
 * @{
 */
/// Maximum number of visible satellites
#define NMEA_CORE_MAX_VISIBLE_SVS 40

/// Maximum size of the NMEA packet buffer
#define NMEA_PACKET_MAX_SIZE 128

/// Size of the NMEA packet checksum
#define NMEA_PACKET_CHECKSUM_SIZE 2

/// Maximum size of the packet header name
#define NMEA_CORE_MESSAGE_ID_SIZE 10
/** @} */

/// Maximum number of satellites that can be used in a GPS fix
#define NMEA_CORE_MAX_USED_GPS 64

/// Maximum number of satellites that can be used in a GLONASS fix
#define NMEA_CORE_MAX_USED_GLO 32

/// Callback invoked when an NMEA packet has been successfully extracted
typedef void (*nmea_core_callback_t)(uint8_t *buff, uint16_t size);

/// NMEA GGA packet with position information
typedef struct
{
  /// Packet header
  char message_id[NMEA_CORE_MESSAGE_ID_SIZE];

  /// UTC time in hhmmss.s-s format
  float nmea_time;

  /// Latitude in ggmm.mmmmm format
  double latitude;

  /// Hemisphere: (N) north, (S) south
  char n_s_indicator;

  /// Longitude in ggmm.mmmmm format
  double longtitude;

  /// Hemisphere: (E) east, (W) west
  char e_w_indicator;

  /// GPS fix status
  int fix_indicator;

  /// Number of satellites used in the GPS fix
  int used_satellites_number;

  /// Horizontal dilution of precision, dimensionless
  float hdop;

  /// Altitude above mean sea level, [m]
  float msl_altitude;

  /// Altitude units (meters)
  char altitude_units;

  /// Geoid separation (difference between the WGS-84 ellipsoid and
  /// mean sea level (geoid)), [m]
  /// @note A '-' sign means the geoid (mean sea level) is below the WGS-84 ellipsoid
  float diff_ellipsoid_and_msl;

  /// Geoid separation units (meters)
  char geoid_units;

  /// Age of differential corrections, [s]
  int age_corrections;

  /// Differential reference station ID.
  int ref_station_id;
} nmea_core_packet_gga_t;

/// NMEA RMC packet with position information
typedef struct
{
  /// Number of correctly received packets
  uint32_t count;

  /// Packet header
  char message_id[NMEA_CORE_MESSAGE_ID_SIZE];

  /// Date in DDMMYY format
  int nmea_date;

  /// UTC time in hhmmss.s-s format
  float nmea_time;

  /// Data validity status: (A) valid, (V) not valid
  char status;

  /// Latitude in ggmm.mmmmm format
  double latitude;

  /// Hemisphere: (N) north, (S) south
  char n_s_indicator;

  /// Longitude in gggmm.mmmmm format
  double longitude;

  /// Hemisphere: (E) east, (W) west
  char e_w_indicator;

  /// Horizontal speed, [knots]
  float horizontal_speed;

  /// Heading, [degrees]
  float heading;

  /// Magnetic declination, [degrees]
  float mag_dec;

  /// Direction: east (E) or west (W)
  char mag_dec_dir;

  /// GPS fix status
  char mode_indicator;
} nmea_core_packet_rmc_t;

/// GSA packet describing the satellites used in the fix,
/// the GPS fix quality and the dilution of precision factors
typedef struct
{
  /// Packet header
  char message_id[NMEA_CORE_MESSAGE_ID_SIZE];

  /// Fix mode: [M] - manually forced to 3D,
  ///           [A] - automatic switching between 3D and 2D modes
  char mode_indicator;

  /// Current GPS fix quality
  int current_mode;

  /// IDs of the satellites in use
  int gps_pnr_used[NMEA_CORE_MAX_USED_GPS];

  int glo_pnr_used[NMEA_CORE_MAX_USED_GLO];

  /// Position dilution of precision
  float pdop;

  /// Horizontal dilution of precision
  float hdop;

  /// Vertical dilution of precision
  float vdop;
} nmea_core_packet_gsa_t;

/// State machine for processing the NMEA stream
typedef struct
{
  /// Current state of the state machine
  enum
  {
    NMEA_CORE_RX__WAIT_START,
    NMEA_CORE_RX__WAIT_DATA,
    NMEA_CORE_RX__WAIT_CHECKSUM,
    NMEA_CORE_RX__WAIT_END,
  } state;

  /// Buffer for the NMEA packet being received
  uint8_t buf[NMEA_PACKET_MAX_SIZE];

  /// Character index in the receive buffer
  uint16_t idx;

  /// Packet checksum
  uint8_t checksum[NMEA_PACKET_CHECKSUM_SIZE];

  /// Index for receiving the checksum
  uint8_t checksum_idx;

  /// Result of parsing the GGA packet with position information
  nmea_core_packet_gga_t gga;

  /// Result of parsing the RMC packet with position information
  nmea_core_packet_rmc_t rmc;

  /// GPS fix quality and satellites in use
  nmea_core_packet_gsa_t gsa;

  /// Callback invoked when an NMEA packet is extracted
  nmea_core_callback_t got_packet;

  struct
  {
    /// Number of successfully received packets
    uint32_t success_packets;

    /// Number of packets received with errors
    uint32_t error_packets;

    /// Timestamp of the last successfully received packet
    uint32_t rx_success_timestamp;
  } stats;
} nmea_core_rx_t;

void nmea_core_init(nmea_core_rx_t *nmea_core);
void nmea_core_rx_process(nmea_core_rx_t *nmea_core, uint8_t symbol);
error_t nmea_core_create_message(char *packet, uint16_t max_size, char *data, uint16_t size);
uint32_t nmea_core_convert_time_to_unix(float nmea_time, int nmea_date);
