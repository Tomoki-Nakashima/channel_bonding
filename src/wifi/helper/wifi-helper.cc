/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/wifi-net-device.h"
#include "ns3/minstrel-wifi-manager.h"
#include "ns3/minstrel-ht-wifi-manager.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/ampdu-subframe-header.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/radiotap-header.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/qos-utils.h"
#include "ns3/ht-configuration.h"
#include "ns3/vht-configuration.h"
#include "ns3/he-configuration.h"
#include "ns3/obss-pd-algorithm.h"
#include "ns3/wifi-ack-policy-selector.h"
#include "ns3/channel-bonding-manager.h"
#include "wifi-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiHelper");

/**
 * ASCII trace Phy transmit sink with context
 * \param stream the output stream
 * \param context the context name
 * \param p the packet
 * \param mode the wifi mode
 * \param preamble the wifi preamble
 * \param txLevel the transmit power level
 */
static void
AsciiPhyTransmitSinkWithContext (
  Ptr<OutputStreamWrapper> stream,
  std::string context,
  Ptr<const Packet> p,
  WifiMode mode,
  WifiPreamble preamble,
  uint8_t txLevel)
{
  NS_LOG_FUNCTION (stream << context << p << mode << preamble << txLevel);
  *stream->GetStream () << "t " << Simulator::Now ().GetSeconds () << " " << context << " " << mode << " " << *p << std::endl;
}

/**
 * ASCII trace Phy transmit sink without context
 * \param stream the output stream
 * \param p the packet
 * \param mode the wifi mode
 * \param preamble the wifi preamble
 * \param txLevel the transmit power level
 */
static void
AsciiPhyTransmitSinkWithoutContext (
  Ptr<OutputStreamWrapper> stream,
  Ptr<const Packet> p,
  WifiMode mode,
  WifiPreamble preamble,
  uint8_t txLevel)
{
  NS_LOG_FUNCTION (stream << p << mode << preamble << txLevel);
  *stream->GetStream () << "t " << Simulator::Now ().GetSeconds () << " " << mode << " " << *p << std::endl;
}

/**
 * ASCII trace Phy receive sink with context
 * \param stream the output stream
 * \param context the context name
 * \param p the packet
 * \param snr the SNR
 * \param mode the wifi mode
 * \param preamble the wifi preamble
 */
static void
AsciiPhyReceiveSinkWithContext (
  Ptr<OutputStreamWrapper> stream,
  std::string context,
  Ptr<const Packet> p,
  double snr,
  WifiMode mode,
  WifiPreamble preamble)
{
  NS_LOG_FUNCTION (stream << context << p << snr << mode << preamble);
  *stream->GetStream () << "r " << Simulator::Now ().GetSeconds () << " " << mode << "" << context << " " << *p << std::endl;
}

/**
 * ASCII trace Phy receive sink without context
 * \param stream the output stream
 * \param p the packet
 * \param snr the SNR
 * \param mode the wifi mode
 * \param preamble the wifi preamble
 */
static void
AsciiPhyReceiveSinkWithoutContext (
  Ptr<OutputStreamWrapper> stream,
  Ptr<const Packet> p,
  double snr,
  WifiMode mode,
  WifiPreamble preamble)
{
  NS_LOG_FUNCTION (stream << p << snr << mode << preamble);
  *stream->GetStream () << "r " << Simulator::Now ().GetSeconds () << " " << mode << " " << *p << std::endl;
}

WifiPhyHelper::WifiPhyHelper ()
  : m_pcapDlt (PcapHelper::DLT_IEEE802_11)
{
  SetPreambleDetectionModel ("ns3::ThresholdPreambleDetectionModel");
}

WifiPhyHelper::~WifiPhyHelper ()
{
}

void
WifiPhyHelper::Set (std::string name, const AttributeValue &v)
{
  m_phy.Set (name, v);
}

void
WifiPhyHelper::SetErrorRateModel (std::string name,
                                  std::string n0, const AttributeValue &v0,
                                  std::string n1, const AttributeValue &v1,
                                  std::string n2, const AttributeValue &v2,
                                  std::string n3, const AttributeValue &v3,
                                  std::string n4, const AttributeValue &v4,
                                  std::string n5, const AttributeValue &v5,
                                  std::string n6, const AttributeValue &v6,
                                  std::string n7, const AttributeValue &v7)
{
  m_errorRateModel = ObjectFactory ();
  m_errorRateModel.SetTypeId (name);
  m_errorRateModel.Set (n0, v0);
  m_errorRateModel.Set (n1, v1);
  m_errorRateModel.Set (n2, v2);
  m_errorRateModel.Set (n3, v3);
  m_errorRateModel.Set (n4, v4);
  m_errorRateModel.Set (n5, v5);
  m_errorRateModel.Set (n6, v6);
  m_errorRateModel.Set (n7, v7);
}

void
WifiPhyHelper::SetFrameCaptureModel (std::string name,
                                     std::string n0, const AttributeValue &v0,
                                     std::string n1, const AttributeValue &v1,
                                     std::string n2, const AttributeValue &v2,
                                     std::string n3, const AttributeValue &v3,
                                     std::string n4, const AttributeValue &v4,
                                     std::string n5, const AttributeValue &v5,
                                     std::string n6, const AttributeValue &v6,
                                     std::string n7, const AttributeValue &v7)
{
  m_frameCaptureModel = ObjectFactory ();
  m_frameCaptureModel.SetTypeId (name);
  m_frameCaptureModel.Set (n0, v0);
  m_frameCaptureModel.Set (n1, v1);
  m_frameCaptureModel.Set (n2, v2);
  m_frameCaptureModel.Set (n3, v3);
  m_frameCaptureModel.Set (n4, v4);
  m_frameCaptureModel.Set (n5, v5);
  m_frameCaptureModel.Set (n6, v6);
  m_frameCaptureModel.Set (n7, v7);
}

void
WifiPhyHelper::SetPreambleDetectionModel (std::string name,
                                          std::string n0, const AttributeValue &v0,
                                          std::string n1, const AttributeValue &v1,
                                          std::string n2, const AttributeValue &v2,
                                          std::string n3, const AttributeValue &v3,
                                          std::string n4, const AttributeValue &v4,
                                          std::string n5, const AttributeValue &v5,
                                          std::string n6, const AttributeValue &v6,
                                          std::string n7, const AttributeValue &v7)
{
  m_preambleDetectionModel = ObjectFactory ();
  m_preambleDetectionModel.SetTypeId (name);
  m_preambleDetectionModel.Set (n0, v0);
  m_preambleDetectionModel.Set (n1, v1);
  m_preambleDetectionModel.Set (n2, v2);
  m_preambleDetectionModel.Set (n3, v3);
  m_preambleDetectionModel.Set (n4, v4);
  m_preambleDetectionModel.Set (n5, v5);
  m_preambleDetectionModel.Set (n6, v6);
  m_preambleDetectionModel.Set (n7, v7);
}

void
WifiPhyHelper::DisablePreambleDetectionModel ()
{
    m_preambleDetectionModel.SetTypeId (TypeId ());
}

void
WifiPhyHelper::PcapSniffTxEvent (
  Ptr<PcapFileWrapper> file,
  Ptr<const Packet>    packet,
  uint16_t             channelFreqMhz,
  WifiTxVector         txVector,
  MpduInfo             aMpdu)
{
  uint32_t dlt = file->GetDataLinkType ();
  switch (dlt)
    {
    case PcapHelper::DLT_IEEE802_11:
      file->Write (Simulator::Now (), packet);
      return;
    case PcapHelper::DLT_PRISM_HEADER:
      {
        NS_FATAL_ERROR ("PcapSniffTxEvent(): DLT_PRISM_HEADER not implemented");
        return;
      }
    case PcapHelper::DLT_IEEE802_11_RADIO:
      {
        Ptr<Packet> p = packet->Copy ();
        RadiotapHeader header = GetRadiotapHeader (p, channelFreqMhz, txVector, aMpdu);
        p->AddHeader (header);
        file->Write (Simulator::Now (), p);
        return;
      }
    default:
      NS_ABORT_MSG ("PcapSniffTxEvent(): Unexpected data link type " << dlt);
    }
}

void
WifiPhyHelper::PcapSniffRxEvent (
  Ptr<PcapFileWrapper>  file,
  Ptr<const Packet>     packet,
  uint16_t              channelFreqMhz,
  WifiTxVector          txVector,
  MpduInfo              aMpdu,
  SignalNoiseDbm        signalNoise)
{
  uint32_t dlt = file->GetDataLinkType ();
  switch (dlt)
    {
    case PcapHelper::DLT_IEEE802_11:
      file->Write (Simulator::Now (), packet);
      return;
    case PcapHelper::DLT_PRISM_HEADER:
      {
        NS_FATAL_ERROR ("PcapSniffRxEvent(): DLT_PRISM_HEADER not implemented");
        return;
      }
    case PcapHelper::DLT_IEEE802_11_RADIO:
      {
        Ptr<Packet> p = packet->Copy ();
        RadiotapHeader header = GetRadiotapHeader (p, channelFreqMhz, txVector, aMpdu);
        header.SetAntennaSignalPower (signalNoise.signal);
        header.SetAntennaNoisePower (signalNoise.noise);
        p->AddHeader (header);
        file->Write (Simulator::Now (), p);
        return;
      }
    default:
      NS_ABORT_MSG ("PcapSniffRxEvent(): Unexpected data link type " << dlt);
    }
}

RadiotapHeader
WifiPhyHelper::GetRadiotapHeader (
  Ptr<Packet>          packet,
  uint16_t             channelFreqMhz,
  WifiTxVector         txVector,
  MpduInfo             aMpdu)
{
  RadiotapHeader header;
  WifiPreamble preamble = txVector.GetPreambleType ();

  uint8_t frameFlags = RadiotapHeader::FRAME_FLAG_NONE;
  header.SetTsft (Simulator::Now ().GetMicroSeconds ());

  //Our capture includes the FCS, so we set the flag to say so.
  frameFlags |= RadiotapHeader::FRAME_FLAG_FCS_INCLUDED;

  if (preamble == WIFI_PREAMBLE_SHORT)
    {
      frameFlags |= RadiotapHeader::FRAME_FLAG_SHORT_PREAMBLE;
    }

  if (txVector.GetGuardInterval () == 400)
    {
      frameFlags |= RadiotapHeader::FRAME_FLAG_SHORT_GUARD;
    }

  header.SetFrameFlags (frameFlags);

  uint64_t rate = 0;
  if (txVector.GetMode ().GetModulationClass () != WIFI_MOD_CLASS_HT
      && txVector.GetMode ().GetModulationClass () != WIFI_MOD_CLASS_VHT
      && txVector.GetMode ().GetModulationClass () != WIFI_MOD_CLASS_HE)
    {
      rate = txVector.GetMode ().GetDataRate (txVector.GetChannelWidth (), txVector.GetGuardInterval (), 1) * txVector.GetNss () / 500000;
      header.SetRate (static_cast<uint8_t> (rate));
    }

  uint16_t channelFlags = 0;
  switch (rate)
    {
    case 2:  //1Mbps
    case 4:  //2Mbps
    case 10: //5Mbps
    case 22: //11Mbps
      channelFlags |= RadiotapHeader::CHANNEL_FLAG_CCK;
      break;
    default:
      channelFlags |= RadiotapHeader::CHANNEL_FLAG_OFDM;
      break;
    }

  if (channelFreqMhz < 2500)
    {
      channelFlags |= RadiotapHeader::CHANNEL_FLAG_SPECTRUM_2GHZ;
    }
  else
    {
      channelFlags |= RadiotapHeader::CHANNEL_FLAG_SPECTRUM_5GHZ;
    }

  header.SetChannelFrequencyAndFlags (channelFreqMhz, channelFlags);

  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HT)
    {
      uint8_t mcsKnown = RadiotapHeader::MCS_KNOWN_NONE;
      uint8_t mcsFlags = RadiotapHeader::MCS_FLAGS_NONE;

      mcsKnown |= RadiotapHeader::MCS_KNOWN_INDEX;

      mcsKnown |= RadiotapHeader::MCS_KNOWN_BANDWIDTH;
      if (txVector.GetChannelWidth () == 40)
        {
          mcsFlags |= RadiotapHeader::MCS_FLAGS_BANDWIDTH_40;
        }

      mcsKnown |= RadiotapHeader::MCS_KNOWN_GUARD_INTERVAL;
      if (txVector.GetGuardInterval () == 400)
        {
          mcsFlags |= RadiotapHeader::MCS_FLAGS_GUARD_INTERVAL;
        }

      mcsKnown |= RadiotapHeader::MCS_KNOWN_HT_FORMAT;
      if (preamble == WIFI_PREAMBLE_HT_GF)
        {
          mcsFlags |= RadiotapHeader::MCS_FLAGS_HT_GREENFIELD;
        }

      mcsKnown |= RadiotapHeader::MCS_KNOWN_NESS;
      if (txVector.GetNess () & 0x01) //bit 1
        {
          mcsFlags |= RadiotapHeader::MCS_FLAGS_NESS_BIT_0;
        }
      if (txVector.GetNess () & 0x02) //bit 2
        {
          mcsKnown |= RadiotapHeader::MCS_KNOWN_NESS_BIT_1;
        }

      mcsKnown |= RadiotapHeader::MCS_KNOWN_FEC_TYPE; //only BCC is currently supported

      mcsKnown |= RadiotapHeader::MCS_KNOWN_STBC;
      if (txVector.IsStbc ())
        {
          mcsFlags |= RadiotapHeader::MCS_FLAGS_STBC_STREAMS;
        }

      header.SetMcsFields (mcsKnown, mcsFlags, txVector.GetMode ().GetMcsValue ());
    }

  if (txVector.IsAggregation ())
    {
      uint16_t ampduStatusFlags = RadiotapHeader::A_MPDU_STATUS_NONE;
      ampduStatusFlags |= RadiotapHeader::A_MPDU_STATUS_LAST_KNOWN;
      /* For PCAP file, MPDU Delimiter and Padding should be removed by the MAC Driver */
      AmpduSubframeHeader hdr;
      uint32_t extractedLength;
      packet->RemoveHeader (hdr);
      extractedLength = hdr.GetLength ();
      packet = packet->CreateFragment (0, static_cast<uint32_t> (extractedLength));
      if (aMpdu.type == LAST_MPDU_IN_AGGREGATE || (hdr.GetEof () == true && hdr.GetLength () > 0))
        {
          ampduStatusFlags |= RadiotapHeader::A_MPDU_STATUS_LAST;
        }
      header.SetAmpduStatus (aMpdu.mpduRefNumber, ampduStatusFlags, 1 /*CRC*/);
    }

  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      uint16_t vhtKnown = RadiotapHeader::VHT_KNOWN_NONE;
      uint8_t vhtFlags = RadiotapHeader::VHT_FLAGS_NONE;
      uint8_t vhtBandwidth = 0;
      uint8_t vhtMcsNss[4] = {0,0,0,0};
      uint8_t vhtCoding = 0;
      uint8_t vhtGroupId = 0;
      uint16_t vhtPartialAid = 0;

      vhtKnown |= RadiotapHeader::VHT_KNOWN_STBC;
      if (txVector.IsStbc ())
        {
          vhtFlags |= RadiotapHeader::VHT_FLAGS_STBC;
        }

      vhtKnown |= RadiotapHeader::VHT_KNOWN_GUARD_INTERVAL;
      if (txVector.GetGuardInterval () == 400)
        {
          vhtFlags |= RadiotapHeader::VHT_FLAGS_GUARD_INTERVAL;
        }

      vhtKnown |= RadiotapHeader::VHT_KNOWN_BEAMFORMED; //Beamforming is currently not supported

      vhtKnown |= RadiotapHeader::VHT_KNOWN_BANDWIDTH;
      //not all bandwidth values are currently supported
      if (txVector.GetChannelWidth () == 40)
        {
          vhtBandwidth = 1;
        }
      else if (txVector.GetChannelWidth () == 80)
        {
          vhtBandwidth = 4;
        }
      else if (txVector.GetChannelWidth () == 160)
        {
          vhtBandwidth = 11;
        }

      //only SU PPDUs are currently supported
      vhtMcsNss[0] |= (txVector.GetNss () & 0x0f);
      vhtMcsNss[0] |= ((txVector.GetMode ().GetMcsValue () << 4) & 0xf0);

      header.SetVhtFields (vhtKnown, vhtFlags, vhtBandwidth, vhtMcsNss, vhtCoding, vhtGroupId, vhtPartialAid);
    }

  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      uint16_t data1 = RadiotapHeader::HE_DATA1_STBC_KNOWN | RadiotapHeader::HE_DATA1_DATA_MCS_KNOWN;
      if (preamble == WIFI_PREAMBLE_HE_ER_SU)
        {
          data1 |= RadiotapHeader::HE_DATA1_FORMAT_EXT_SU;
        }
      else if (preamble == WIFI_PREAMBLE_HE_MU)
        {
          data1 |= RadiotapHeader::HE_DATA1_FORMAT_MU;
        }
      else if (preamble == WIFI_PREAMBLE_HE_TB)
        {
          data1 |= RadiotapHeader::HE_DATA1_FORMAT_TRIG;
        }

      uint16_t data2 = RadiotapHeader::HE_DATA2_NUM_LTF_SYMS_KNOWN | RadiotapHeader::HE_DATA2_GI_KNOWN;

      uint16_t data3 = 0;
      if (txVector.IsStbc ())
        {
          data3 |= RadiotapHeader::HE_DATA3_STBC;
        }

      uint16_t data5 = 0;
      if (txVector.GetChannelWidth () == 40)
        {
          data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_40MHZ;
        }
      else if (txVector.GetChannelWidth () == 80)
        {
          data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_80MHZ;
        }
      else if (txVector.GetChannelWidth () == 160)
        {
          data5 |= RadiotapHeader::HE_DATA5_DATA_BW_RU_ALLOC_160MHZ;
        }
      if (txVector.GetGuardInterval () == 1600)
        {
          data5 |= RadiotapHeader::HE_DATA5_GI_1_6;
        }
      else if (txVector.GetGuardInterval () == 3200)
        {
          data5 |= RadiotapHeader::HE_DATA5_GI_3_2;
        }

      header.SetHeFields (data1, data2, data3, data5);
    }

  return header;
}

void
WifiPhyHelper::SetPcapDataLinkType (SupportedPcapDataLinkTypes dlt)
{
  switch (dlt)
    {
    case DLT_IEEE802_11:
      m_pcapDlt = PcapHelper::DLT_IEEE802_11;
      return;
    case DLT_PRISM_HEADER:
      m_pcapDlt = PcapHelper::DLT_PRISM_HEADER;
      return;
    case DLT_IEEE802_11_RADIO:
      m_pcapDlt = PcapHelper::DLT_IEEE802_11_RADIO;
      return;
    default:
      NS_ABORT_MSG ("WifiPhyHelper::SetPcapFormat(): Unexpected format");
    }
}

PcapHelper::DataLinkType
WifiPhyHelper::GetPcapDataLinkType (void) const
{
  return m_pcapDlt;
}

void
WifiPhyHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  NS_LOG_FUNCTION (this << prefix << nd << promiscuous << explicitFilename);

  //All of the Pcap enable functions vector through here including the ones
  //that are wandering through all of devices on perhaps all of the nodes in
  //the system. We can only deal with devices of type WifiNetDevice.
  Ptr<WifiNetDevice> device = nd->GetObject<WifiNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("WifiHelper::EnablePcapInternal(): Device " << &device << " not of type ns3::WifiNetDevice");
      return;
    }

  Ptr<WifiPhy> phy = device->GetPhy ();
  NS_ABORT_MSG_IF (phy == 0, "WifiPhyHelper::EnablePcapInternal(): Phy layer in WifiNetDevice must be set");

  PcapHelper pcapHelper;

  std::string filename;
  if (explicitFilename)
    {
      filename = prefix;
    }
  else
    {
      filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    }

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, m_pcapDlt);

  phy->TraceConnectWithoutContext ("MonitorSnifferTx", MakeBoundCallback (&WifiPhyHelper::PcapSniffTxEvent, file));
  phy->TraceConnectWithoutContext ("MonitorSnifferRx", MakeBoundCallback (&WifiPhyHelper::PcapSniffRxEvent, file));
}

void
WifiPhyHelper::EnableAsciiInternal (
  Ptr<OutputStreamWrapper> stream,
  std::string prefix,
  Ptr<NetDevice> nd,
  bool explicitFilename)
{
  //All of the ascii enable functions vector through here including the ones
  //that are wandering through all of devices on perhaps all of the nodes in
  //the system. We can only deal with devices of type WifiNetDevice.
  Ptr<WifiNetDevice> device = nd->GetObject<WifiNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("WifiHelper::EnableAsciiInternal(): Device " << device << " not of type ns3::WifiNetDevice");
      return;
    }

  //Our trace sinks are going to use packet printing, so we have to make sure
  //that is turned on.
  Packet::EnablePrinting ();

  uint32_t nodeid = nd->GetNode ()->GetId ();
  uint32_t deviceid = nd->GetIfIndex ();
  std::ostringstream oss;

  //If we are not provided an OutputStreamWrapper, we are expected to create
  //one using the usual trace filename conventions and write our traces
  //without a context since there will be one file per context and therefore
  //the context would be redundant.
  if (stream == 0)
    {
      //Set up an output stream object to deal with private ofstream copy
      //constructor and lifetime issues. Let the helper decide the actual
      //name of the file given the prefix.
      AsciiTraceHelper asciiTraceHelper;

      std::string filename;
      if (explicitFilename)
        {
          filename = prefix;
        }
      else
        {
          filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
        }

      Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);
      //We could go poking through the phy and the state looking for the
      //correct trace source, but we can let Config deal with that with
      //some search cost.  Since this is presumably happening at topology
      //creation time, it doesn't seem much of a price to pay.
      oss.str ("");
      oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::WifiNetDevice/Phy/State/RxOk";
      Config::ConnectWithoutContext (oss.str (), MakeBoundCallback (&AsciiPhyReceiveSinkWithoutContext, theStream));

      oss.str ("");
      oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::WifiNetDevice/Phy/State/Tx";
      Config::ConnectWithoutContext (oss.str (), MakeBoundCallback (&AsciiPhyTransmitSinkWithoutContext, theStream));

      return;
    }

  //If we are provided an OutputStreamWrapper, we are expected to use it, and
  //to provide a context. We are free to come up with our own context if we
  //want, and use the AsciiTraceHelper Hook*WithContext functions, but for
  //compatibility and simplicity, we just use Config::Connect and let it deal
  //with coming up with a context.
  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::WifiNetDevice/Phy/State/RxOk";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiPhyReceiveSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::WifiNetDevice/Phy/State/Tx";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiPhyTransmitSinkWithContext, stream));
}

WifiHelper::~WifiHelper ()
{
}

WifiHelper::WifiHelper ()
  : m_standard (WIFI_PHY_STANDARD_80211a),
    m_selectQueueCallback (&SelectQueueByDSField)
{
  SetRemoteStationManager ("ns3::ArfWifiManager");
  SetChannelBondingManager ("ns3::StaticChannelBondingManager");
  SetAckPolicySelectorForAc (AC_BE, "ns3::ConstantWifiAckPolicySelector");
  SetAckPolicySelectorForAc (AC_BK, "ns3::ConstantWifiAckPolicySelector");
  SetAckPolicySelectorForAc (AC_VI, "ns3::ConstantWifiAckPolicySelector");
  SetAckPolicySelectorForAc (AC_VO, "ns3::ConstantWifiAckPolicySelector");
}

void
WifiHelper::SetRemoteStationManager (std::string type,
                                     std::string n0, const AttributeValue &v0,
                                     std::string n1, const AttributeValue &v1,
                                     std::string n2, const AttributeValue &v2,
                                     std::string n3, const AttributeValue &v3,
                                     std::string n4, const AttributeValue &v4,
                                     std::string n5, const AttributeValue &v5,
                                     std::string n6, const AttributeValue &v6,
                                     std::string n7, const AttributeValue &v7)
{
  m_stationManager = ObjectFactory ();
  m_stationManager.SetTypeId (type);
  m_stationManager.Set (n0, v0);
  m_stationManager.Set (n1, v1);
  m_stationManager.Set (n2, v2);
  m_stationManager.Set (n3, v3);
  m_stationManager.Set (n4, v4);
  m_stationManager.Set (n5, v5);
  m_stationManager.Set (n6, v6);
  m_stationManager.Set (n7, v7);
}

void
WifiHelper::SetObssPdAlgorithm (std::string type,
                                std::string n0, const AttributeValue &v0,
                                std::string n1, const AttributeValue &v1,
                                std::string n2, const AttributeValue &v2,
                                std::string n3, const AttributeValue &v3,
                                std::string n4, const AttributeValue &v4,
                                std::string n5, const AttributeValue &v5,
                                std::string n6, const AttributeValue &v6,
                                std::string n7, const AttributeValue &v7)
{
  m_obssPdAlgorithm = ObjectFactory ();
  m_obssPdAlgorithm.SetTypeId (type);
  m_obssPdAlgorithm.Set (n0, v0);
  m_obssPdAlgorithm.Set (n1, v1);
  m_obssPdAlgorithm.Set (n2, v2);
  m_obssPdAlgorithm.Set (n3, v3);
  m_obssPdAlgorithm.Set (n4, v4);
  m_obssPdAlgorithm.Set (n5, v5);
  m_obssPdAlgorithm.Set (n6, v6);
  m_obssPdAlgorithm.Set (n7, v7);
}

void
WifiHelper::SetAckPolicySelectorForAc (AcIndex ac, std::string type,
                                       std::string n0, const AttributeValue &v0,
                                       std::string n1, const AttributeValue &v1,
                                       std::string n2, const AttributeValue &v2,
                                       std::string n3, const AttributeValue &v3,
                                       std::string n4, const AttributeValue &v4,
                                       std::string n5, const AttributeValue &v5,
                                       std::string n6, const AttributeValue &v6,
                                       std::string n7, const AttributeValue &v7)
{
  m_ackPolicySelector[ac] = ObjectFactory ();
  m_ackPolicySelector[ac].SetTypeId (type);
  m_ackPolicySelector[ac].Set (n0, v0);
  m_ackPolicySelector[ac].Set (n1, v1);
  m_ackPolicySelector[ac].Set (n2, v2);
  m_ackPolicySelector[ac].Set (n3, v3);
  m_ackPolicySelector[ac].Set (n4, v4);
  m_ackPolicySelector[ac].Set (n5, v5);
  m_ackPolicySelector[ac].Set (n6, v6);
  m_ackPolicySelector[ac].Set (n7, v7);
}

void
WifiHelper::SetChannelBondingManager (std::string type,
                                      std::string n0, const AttributeValue &v0,
                                      std::string n1, const AttributeValue &v1,
                                      std::string n2, const AttributeValue &v2,
                                      std::string n3, const AttributeValue &v3,
                                      std::string n4, const AttributeValue &v4,
                                      std::string n5, const AttributeValue &v5,
                                      std::string n6, const AttributeValue &v6,
                                      std::string n7, const AttributeValue &v7)
{
  m_channelBondingManager = ObjectFactory ();
  m_channelBondingManager.SetTypeId (type);
  m_channelBondingManager.Set (n0, v0);
  m_channelBondingManager.Set (n1, v1);
  m_channelBondingManager.Set (n2, v2);
  m_channelBondingManager.Set (n3, v3);
  m_channelBondingManager.Set (n4, v4);
  m_channelBondingManager.Set (n5, v5);
  m_channelBondingManager.Set (n6, v6);
  m_channelBondingManager.Set (n7, v7);
}

void
WifiHelper::SetStandard (WifiPhyStandard standard)
{
  m_standard = standard;
}

void
WifiHelper::SetSelectQueueCallback (SelectQueueCallback f)
{
  m_selectQueueCallback = f;
}

NetDeviceContainer
WifiHelper::Install (const WifiPhyHelper &phyHelper,
                     const WifiMacHelper &macHelper,
                     NodeContainer::Iterator first,
                     NodeContainer::Iterator last) const
{
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = first; i != last; ++i)
    {
      Ptr<Node> node = *i;
      Ptr<WifiNetDevice> device = CreateObject<WifiNetDevice> ();
      if (m_standard >= WIFI_PHY_STANDARD_80211n_2_4GHZ)
        {
          Ptr<HtConfiguration> htConfiguration = CreateObject<HtConfiguration> ();
          device->SetHtConfiguration (htConfiguration);
        }
      if ((m_standard == WIFI_PHY_STANDARD_80211ac) || (m_standard == WIFI_PHY_STANDARD_80211ax_5GHZ))
        {
          Ptr<VhtConfiguration> vhtConfiguration = CreateObject<VhtConfiguration> ();
          device->SetVhtConfiguration (vhtConfiguration);
        }
      if (m_standard >= WIFI_PHY_STANDARD_80211ax_2_4GHZ)
        {
          Ptr<HeConfiguration> heConfiguration = CreateObject<HeConfiguration> ();
          device->SetHeConfiguration (heConfiguration);
        }
      Ptr<WifiRemoteStationManager> manager = m_stationManager.Create<WifiRemoteStationManager> ();
      Ptr<WifiMac> mac = macHelper.Create (device);
      Ptr<WifiPhy> phy = phyHelper.Create (node, device);
      mac->SetAddress (Mac48Address::Allocate ());
      mac->ConfigureStandard (m_standard);
      phy->ConfigureStandard (m_standard);
      device->SetMac (mac);
      device->SetPhy (phy);
      device->SetRemoteStationManager (manager);
      node->AddDevice (device);
      if ((m_standard >= WIFI_PHY_STANDARD_80211ax_2_4GHZ) && (m_obssPdAlgorithm.IsTypeIdSet ()))
        {
          Ptr<ObssPdAlgorithm> obssPdAlgorithm = m_obssPdAlgorithm.Create<ObssPdAlgorithm> ();
          device->AggregateObject (obssPdAlgorithm);
          obssPdAlgorithm->ConnectWifiNetDevice (device);
        }
      if ((m_standard >= WIFI_PHY_STANDARD_80211n_2_4GHZ) && (m_channelBondingManager.IsTypeIdSet ()))
        {
          Ptr<ChannelBondingManager> channelBondingManager = m_channelBondingManager.Create<ChannelBondingManager> ();
          phy->SetChannelBondingManager (channelBondingManager);
        }
      devices.Add (device);
      NS_LOG_DEBUG ("node=" << node << ", mob=" << node->GetObject<MobilityModel> ());
      // Aggregate a NetDeviceQueueInterface object if a RegularWifiMac is installed
      Ptr<RegularWifiMac> rmac = DynamicCast<RegularWifiMac> (mac);
      if (rmac)
        {
          Ptr<NetDeviceQueueInterface> ndqi;
          BooleanValue qosSupported;
          PointerValue ptr;
          Ptr<WifiMacQueue> wmq;
          Ptr<WifiAckPolicySelector> ackSelector;

          rmac->GetAttributeFailSafe ("QosSupported", qosSupported);
          if (qosSupported.Get ())
            {
              ndqi = CreateObjectWithAttributes<NetDeviceQueueInterface> ("NTxQueues",
                                                                          UintegerValue (4));

              rmac->GetAttributeFailSafe ("BE_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_BE].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (0)->ConnectQueueTraces (wmq);

              rmac->GetAttributeFailSafe ("BK_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_BK].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (1)->ConnectQueueTraces (wmq);

              rmac->GetAttributeFailSafe ("VI_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_VI].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (2)->ConnectQueueTraces (wmq);

              rmac->GetAttributeFailSafe ("VO_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_VO].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (3)->ConnectQueueTraces (wmq);
              ndqi->SetSelectQueueCallback (m_selectQueueCallback);
            }
          else
            {
              ndqi = CreateObject<NetDeviceQueueInterface> ();

              rmac->GetAttributeFailSafe ("Txop", ptr);
              wmq = ptr.Get<Txop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (0)->ConnectQueueTraces (wmq);
            }
          device->AggregateObject (ndqi);
        }
    }
  return devices;
}

NetDeviceContainer
WifiHelper::Install (const WifiPhyHelper &phyHelper,
                     const WifiMacHelper &macHelper, NodeContainer c) const
{
  return Install (phyHelper, macHelper, c.Begin (), c.End ());
}

NetDeviceContainer
WifiHelper::Install (const WifiPhyHelper &phy,
                     const WifiMacHelper &mac, Ptr<Node> node) const
{
  return Install (phy, mac, NodeContainer (node));
}

NetDeviceContainer
WifiHelper::Install (const WifiPhyHelper &phy,
                     const WifiMacHelper &mac, std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (phy, mac, NodeContainer (node));
}

void
WifiHelper::EnableLogComponents (void)
{
  LogComponentEnableAll (LOG_PREFIX_TIME);
  LogComponentEnableAll (LOG_PREFIX_NODE);

  LogComponentEnable ("AarfWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("AarfcdWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("AdhocWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("AmrrWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("ApWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("AparfWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("ArfWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("BlockAckAgreement", LOG_LEVEL_ALL);
  LogComponentEnable ("BlockAckCache", LOG_LEVEL_ALL);
  LogComponentEnable ("BlockAckManager", LOG_LEVEL_ALL);
  LogComponentEnable ("CaraWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("ChannelBondingManager", LOG_LEVEL_ALL);
  LogComponentEnable ("ConstantThresholdChannelBondingManager", LOG_LEVEL_ALL);
  LogComponentEnable ("ConstantObssPdAlgorithm", LOG_LEVEL_ALL);
  LogComponentEnable ("ConstantRateWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("Txop", LOG_LEVEL_ALL);
  LogComponentEnable ("ChannelAccessManager", LOG_LEVEL_ALL);
  LogComponentEnable ("DsssErrorRateModel", LOG_LEVEL_ALL);
  LogComponentEnable ("DynamicThresholdChannelBondingManager", LOG_LEVEL_ALL);
  LogComponentEnable ("QosTxop", LOG_LEVEL_ALL);
  LogComponentEnable ("IdealWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("InfrastructureWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("InterferenceHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("MacLow", LOG_LEVEL_ALL);
  LogComponentEnable ("MacRxMiddle", LOG_LEVEL_ALL);
  LogComponentEnable ("MacTxMiddle", LOG_LEVEL_ALL);
  LogComponentEnable ("MinstrelHtWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("MinstrelWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("MpduAggregator", LOG_LEVEL_ALL);
  LogComponentEnable ("MsduAggregator", LOG_LEVEL_ALL);
  LogComponentEnable ("NistErrorRateModel", LOG_LEVEL_ALL);
  LogComponentEnable ("ObssPdAlgorithm", LOG_LEVEL_ALL);
  LogComponentEnable ("OnoeWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("ParfWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("RegularWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("RraaWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("RrpaaWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("SimpleFrameCaptureModel", LOG_LEVEL_ALL);
  LogComponentEnable ("SpectrumWifiPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("StaticChannelBondingManager", LOG_LEVEL_ALL);
  LogComponentEnable ("StaWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("SupportedRates", LOG_LEVEL_ALL);
  LogComponentEnable ("ThresholdPreambleDetectionModel", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiMacQueueItem", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiPhyStateHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiPpdu", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiPsdu", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiRadioEnergyModel", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiRemoteStationManager", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiSpectrumPhyInterface", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiSpectrumSignalParameters", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiTxCurrentModel", LOG_LEVEL_ALL);
  LogComponentEnable ("YansErrorRateModel", LOG_LEVEL_ALL);
  LogComponentEnable ("YansWifiChannel", LOG_LEVEL_ALL);
  LogComponentEnable ("YansWifiPhy", LOG_LEVEL_ALL);

  //From Spectrum
  LogComponentEnable ("WifiSpectrumValueHelper", LOG_LEVEL_ALL);
}

int64_t
WifiHelper::AssignStreams (NetDeviceContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<NetDevice> netDevice;
  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      netDevice = (*i);
      Ptr<WifiNetDevice> wifi = DynamicCast<WifiNetDevice> (netDevice);
      if (wifi)
        {
          //Handle any random numbers in the PHY objects.
          currentStream += wifi->GetPhy ()->AssignStreams (currentStream);

          //Handle any random numbers in the station managers.
          Ptr<WifiRemoteStationManager> manager = wifi->GetRemoteStationManager ();
          Ptr<MinstrelWifiManager> minstrel = DynamicCast<MinstrelWifiManager> (manager);
          if (minstrel)
            {
              currentStream += minstrel->AssignStreams (currentStream);
            }

          Ptr<MinstrelHtWifiManager> minstrelHt = DynamicCast<MinstrelHtWifiManager> (manager);
          if (minstrelHt)
            {
              currentStream += minstrelHt->AssignStreams (currentStream);
            }

          //Handle any random numbers in the MAC objects.
          Ptr<WifiMac> mac = wifi->GetMac ();
          Ptr<RegularWifiMac> rmac = DynamicCast<RegularWifiMac> (mac);
          if (rmac)
            {
              PointerValue ptr;
              rmac->GetAttribute ("Txop", ptr);
              Ptr<Txop> txop = ptr.Get<Txop> ();
              currentStream += txop->AssignStreams (currentStream);

              rmac->GetAttribute ("VO_Txop", ptr);
              Ptr<QosTxop> vo_txop = ptr.Get<QosTxop> ();
              currentStream += vo_txop->AssignStreams (currentStream);

              rmac->GetAttribute ("VI_Txop", ptr);
              Ptr<QosTxop> vi_txop = ptr.Get<QosTxop> ();
              currentStream += vi_txop->AssignStreams (currentStream);

              rmac->GetAttribute ("BE_Txop", ptr);
              Ptr<QosTxop> be_txop = ptr.Get<QosTxop> ();
              currentStream += be_txop->AssignStreams (currentStream);

              rmac->GetAttribute ("BK_Txop", ptr);
              Ptr<QosTxop> bk_txop = ptr.Get<QosTxop> ();
              currentStream += bk_txop->AssignStreams (currentStream);

              //if an AP, handle any beacon jitter
              Ptr<ApWifiMac> apmac = DynamicCast<ApWifiMac> (rmac);
              if (apmac)
                {
                  currentStream += apmac->AssignStreams (currentStream);
                }
            }
        }
    }
  return (currentStream - stream);
}

} //namespace ns3
