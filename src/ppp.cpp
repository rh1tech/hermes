#include <Arduino.h>
#include "globals.h"

#ifdef ESP8266
#include "netif/ppp/ppp.h"

u32_t ppp_output_cb(ppp_pcb *pcb, unsigned char *data, u32_t len, void *ctx)
{
  if (cmdMode)
  {
    return 0;
  }
  else
  {
    return Serial.write(data, len);
  }
}

void ppp_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
  struct netif *pppif = ppp_netif(pcb);
  LWIP_UNUSED_ARG(ctx);
  switch (err_code)
  {
  case PPPERR_NONE: // No error == connected successfully
#if DEBUG
    Serial.println(F("status_cb: Connected"));
#if PPP_IPV4_SUPPORT
    Serial.print(F("   our_ipaddr  = "));
    Serial.println(ipaddr_ntoa(&pppif->ip_addr));
    Serial.print(F("   his_ipaddr  = "));
    Serial.println(ipaddr_ntoa(&pppif->gw));
    Serial.print(F("   netmask     = "));
    Serial.println(ipaddr_ntoa(&pppif->netmask));
#if LWIP_DNS
    const ip_addr_t *ns;
    ns = dns_getserver(0);
    Serial.print(F("   dns1        = "));
    Serial.println(ipaddr_ntoa(ns));
    ns = dns_getserver(1);
    Serial.print(F("   dns2        = "));
    Serial.println(ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#endif /* PPP_IPV4_SUPPORT */
#if PPP_IPV6_SUPPORT
    // printf("   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
#endif /* DEBUG */
#if NAPT_SUPPORTED
    // Enable NAT-ing this connection (ESP8266 only)
    ip_napt_enable(pppif->ip_addr.addr, 1);
#endif
    break;
  case PPPERR_USER: // Clean disconnect
    // sendString(F("PPP: shutdown"));
    if (ppp_free(ppp) == 0)
    {
      ppp = NULL;
      // sendString(F("PPP: freed"));
    }
    else
    {
      sendString(F("ppp_free() failed"));
    }
    break;
  // Any error other than NONE or USER is considered an error; abort and return to command mode
  case PPPERR_PARAM:
    sendString(F("PPP: Invalid parameter"));
    break;
  case PPPERR_OPEN:
    sendString(F("PPP: Unable to open PPP session"));
    break;
  case PPPERR_DEVICE:
    sendString(F("PPP: Invalid I/O device"));
    break;
  case PPPERR_ALLOC:
    sendString(F("PPP: Unable to allocate resources"));
    break;
  case PPPERR_CONNECT:
    sendString(F("PPP: Connection lost"));
    break;
  case PPPERR_AUTHFAIL:
    sendString(F("PPP: Failed authentication challenge"));
    break;
  case PPPERR_PROTOCOL:
    sendString(F("PPP: Failed to meet protocol"));
    break;
  case PPPERR_PEERDEAD:
    sendString(F("PPP: Connection timeout"));
    break;
  case PPPERR_IDLETIMEOUT:
    sendString(F("PPP: Idle Timeout"));
    break;
  case PPPERR_CONNECTTIME:
    sendString(F("PPP: Max connect time reached"));
    break;
  case PPPERR_LOOPBACK:
    sendString(F("PPP: Loopback detected"));
    break;
  }

  // for any error other than USER or NONE, hang up
  if (!((err_code == PPPERR_USER) || (err_code == PPPERR_NONE)))
  {
    hangUp();
    cmdMode = true;
  }
}

#else // ESP32 or other platforms - stubs only

u32_t ppp_output_cb(void *pcb, unsigned char *data, u32_t len, void *ctx)
{
  return 0;
}

void ppp_status_cb(void *pcb, int err_code, void *ctx)
{
  // Stub for ESP32
}

#endif
