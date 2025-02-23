/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *
 * Ported from yans-wifi-phy.cc by several contributors starting
 * with Nicola Baldo and Dean Armstrong
 */

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "spectrum-wifi-phy.h"
#include "wifi-spectrum-signal-parameters.h"
#include "wifi-spectrum-phy-interface.h"
#include "wifi-utils.h"
#include "wifi-ppdu.h"
#include "wifi-psdu.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SpectrumWifiPhy");

NS_OBJECT_ENSURE_REGISTERED (SpectrumWifiPhy);

TypeId
SpectrumWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpectrumWifiPhy")
    .SetParent<WifiPhy> ()
    .SetGroupName ("Wifi")
    .AddConstructor<SpectrumWifiPhy> ()
    .AddAttribute ("DisableWifiReception",
                   "Prevent Wi-Fi frame sync from ever happening",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SpectrumWifiPhy::m_disableWifiReception),
                   MakeBooleanChecker ())
    .AddAttribute ("TxMaskInnerBandMinimumRejection",
                   "Minimum rejection (dBr) for the inner band of the transmit spectrum mask",
                   DoubleValue (-20.0),
                   MakeDoubleAccessor (&SpectrumWifiPhy::m_txMaskInnerBandMinimumRejection),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMaskOuterBandMinimumRejection",
                   "Minimum rejection (dBr) for the outer band of the transmit spectrum mask",
                   DoubleValue (-28.0),
                   MakeDoubleAccessor (&SpectrumWifiPhy::m_txMaskOuterBandMinimumRejection),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMaskOuterBandMaximumRejection",
                   "Maximum rejection (dBr) for the outer band of the transmit spectrum mask",
                   DoubleValue (-40.0),
                   MakeDoubleAccessor (&SpectrumWifiPhy::m_txMaskOuterBandMaximumRejection),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("SignalArrival",
                     "Signal arrival",
                     MakeTraceSourceAccessor (&SpectrumWifiPhy::m_signalCb),
                     "ns3::SpectrumWifiPhy::SignalArrivalCallback")
  ;
  return tid;
}

SpectrumWifiPhy::SpectrumWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

SpectrumWifiPhy::~SpectrumWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
SpectrumWifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_wifiSpectrumPhyInterface = 0;
  WifiPhy::DoDispose ();
}

void
SpectrumWifiPhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  WifiPhy::DoInitialize ();
  // This connection is deferred until frequency and channel width are set
  if (m_channel && m_wifiSpectrumPhyInterface)
    {
      m_channel->AddRx (m_wifiSpectrumPhyInterface);
    }
  else
    {
      NS_FATAL_ERROR ("SpectrumWifiPhy misses channel and WifiSpectrumPhyInterface objects at initialization time");
    }
}

Ptr<const SpectrumModel>
SpectrumWifiPhy::GetRxSpectrumModel ()
{
  NS_LOG_FUNCTION (this);
  if (m_rxSpectrumModel)
    {
      return m_rxSpectrumModel;
    }
  else
    {
      if (GetFrequency () == 0)
        {
          NS_LOG_DEBUG ("Frequency is not set; returning 0");
          return 0;
        }
      else
        {
          uint16_t channelWidth = GetChannelWidth ();
          NS_LOG_DEBUG ("Creating spectrum model from frequency/width pair of (" << GetFrequency () << ", " << channelWidth << ")");
          m_rxSpectrumModel = WifiSpectrumValueHelper::GetSpectrumModel (GetFrequency (), channelWidth, GetBandBandwidth (), GetGuardBandwidth (channelWidth));
          UpdateInterferenceHelperBands ();
        }
    }
  return m_rxSpectrumModel;
}

void
SpectrumWifiPhy::UpdateInterferenceHelperBands (void)
{
  NS_LOG_FUNCTION (this);
  uint16_t channelWidth = GetChannelWidth ();
  m_interference.RemoveBands ();
  if (channelWidth < 20)
    {
      WifiSpectrumBand band = GetBand (channelWidth);
      m_interference.AddBand (band);
    }
  else
    {
      for (uint8_t i = 0; i < (channelWidth / 20); i++)
        {
          WifiSpectrumBand band = GetBand (20, i);
          m_interference.AddBand (band);
        }
    }
  if ((GetStandard () == WIFI_PHY_STANDARD_80211ax_2_4GHZ) || (GetStandard () == WIFI_PHY_STANDARD_80211ax_5GHZ))
    {
      for (unsigned int type = 0; type < 7; type++)
        {
          HeRu::RuType ruType = static_cast <HeRu::RuType> (type);
          for (std::size_t index = 1; index <= HeRu::GetNRus (channelWidth, ruType); index++)
            {
              HeRu::SubcarrierGroup group = HeRu::GetSubcarrierGroup (channelWidth, ruType, index);
              HeRu::SubcarrierRange range = std::make_pair (group.front ().first, group.back ().second);
              WifiSpectrumBand band = ConvertHeRuSubcarriers (channelWidth, range);
              m_interference.AddBand (band);
            }
        }
    }
}

Ptr<Channel>
SpectrumWifiPhy::GetChannel (void) const
{
  return m_channel;
}

void
SpectrumWifiPhy::SetChannel (const Ptr<SpectrumChannel> channel)
{
  m_channel = channel;
}

void
SpectrumWifiPhy::ResetSpectrumModel (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (IsInitialized (), "Executing method before run-time");
  uint16_t channelWidth = GetChannelWidth ();
  NS_LOG_DEBUG ("Run-time change of spectrum model from frequency/width pair of (" << GetFrequency () << ", " << channelWidth << ")");
  // Replace existing spectrum model with new one, and must call AddRx ()
  // on the SpectrumChannel to provide this new spectrum model to it
  m_rxSpectrumModel = WifiSpectrumValueHelper::GetSpectrumModel (GetFrequency (), channelWidth, GetBandBandwidth (), GetGuardBandwidth (channelWidth));
  m_channel->AddRx (m_wifiSpectrumPhyInterface);
  UpdateInterferenceHelperBands ();
}

void
SpectrumWifiPhy::SetChannelNumber (uint8_t nch)
{
  NS_LOG_FUNCTION (this << +nch);
  WifiPhy::SetChannelNumber (nch);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumWifiPhy::SetFrequency (uint16_t freq)
{
  NS_LOG_FUNCTION (this << freq);
  WifiPhy::SetFrequency (freq);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumWifiPhy::SetChannelWidth (uint16_t channelwidth)
{
  NS_LOG_FUNCTION (this << channelwidth);
  WifiPhy::SetChannelWidth (channelwidth);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumWifiPhy::ConfigureStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  WifiPhy::ConfigureStandard (standard);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumWifiPhy::StartRx (Ptr<SpectrumSignalParameters> rxParams)
{
  NS_LOG_FUNCTION (this << rxParams);
  Time rxDuration = rxParams->duration;
  Ptr<SpectrumValue> receivedSignalPsd = rxParams->psd;
  NS_LOG_DEBUG ("Received signal with PSD " << *receivedSignalPsd << " and duration " << rxDuration.As (Time::NS));
  uint32_t senderNodeId = 0;
  if (rxParams->txPhy && rxParams->txPhy->GetDevice ())
    {
      senderNodeId = rxParams->txPhy->GetDevice ()->GetNode ()->GetId ();
    }
  NS_LOG_DEBUG ("Received signal from " << senderNodeId << " with unfiltered power " << WToDbm (Integral (*receivedSignalPsd)) << " dBm");

  // Integrate over our receive bandwidth (i.e., all that the receive
  // spectral mask representing our filtering allows) to find the
  // total energy apparent to the "demodulator".
  // This is done per 20 MHz channel band.
  uint16_t channelWidth = GetChannelWidth ();
  double totalRxPowerW = 0;
  RxPowerWattPerChannelBand rxPowerW;

  // Since we are using an unordered_map, the order the power is inserted should be respected
  // (i.e. legacy band followed by 11n/ac/ax 20 MHz bands followed by 802.11ax RU bands).
  // This way, we can compute the total RX power by doing a sum over the bands, starting from the first one.
  if ((channelWidth == 5) || (channelWidth == 10))
    {
      WifiSpectrumBand filteredBand = GetBand (channelWidth);
      Ptr<SpectrumValue> filter = WifiSpectrumValueHelper::CreateRfFilter (GetFrequency (), channelWidth, GetBandBandwidth (), GetGuardBandwidth (channelWidth), filteredBand);
      SpectrumValue filteredSignal = (*filter) * (*receivedSignalPsd);
      NS_LOG_DEBUG ("Signal power received (watts) before antenna gain: " << Integral (filteredSignal));
      double rxPowerPerBandW = Integral (filteredSignal) * DbToRatio (GetRxGain ());
      totalRxPowerW += rxPowerPerBandW;
      rxPowerW.push_back (std::make_pair (filteredBand, rxPowerPerBandW));
      NS_LOG_DEBUG ("Signal power received after antenna gain for " << channelWidth << " MHz channel: " << rxPowerPerBandW << " W (" << WToDbm (rxPowerPerBandW) << " dBm)");
    }

  for (uint8_t i = 0; i < (channelWidth / 20); i++)
    {
      WifiSpectrumBand filteredBand = GetBand (20, i);
      Ptr<SpectrumValue> filter = WifiSpectrumValueHelper::CreateRfFilter (GetFrequency (), channelWidth, GetBandBandwidth (), GetGuardBandwidth (channelWidth), filteredBand);
      SpectrumValue filteredSignal = (*filter) * (*receivedSignalPsd);
      NS_LOG_DEBUG ("Signal power received (watts) before antenna gain for 20 MHz channel band " << +i << ": " << Integral (filteredSignal));
      double rxPowerPerBandW = Integral (filteredSignal) * DbToRatio (GetRxGain ());
      totalRxPowerW += rxPowerPerBandW;
      rxPowerW.push_back (std::make_pair (filteredBand, rxPowerPerBandW));
      NS_LOG_DEBUG ("Signal power received after antenna gain for 20 MHz channel band " << +i << ": " << rxPowerPerBandW << " W (" << WToDbm (rxPowerPerBandW) << " dBm)");
    }
  
  if ((GetStandard () == WIFI_PHY_STANDARD_80211ax_2_4GHZ) || (GetStandard () == WIFI_PHY_STANDARD_80211ax_5GHZ))
    {
      for (unsigned int type = 0; type < 7; type++)
        {
          HeRu::RuType ruType = static_cast <HeRu::RuType> (type);
          for (std::size_t index = 1; index <= HeRu::GetNRus (channelWidth, ruType); index++)
            {
              HeRu::SubcarrierGroup group = HeRu::GetSubcarrierGroup (channelWidth, ruType, index);
              HeRu::SubcarrierRange range = std::make_pair (group.front ().first, group.back ().second);
              WifiSpectrumBand band = ConvertHeRuSubcarriers (channelWidth, range);
              Ptr<SpectrumValue> filter = WifiSpectrumValueHelper::CreateRfFilter (GetFrequency (), channelWidth, GetBandBandwidth (), GetGuardBandwidth (channelWidth), band);
              SpectrumValue filteredSignal = (*filter) * (*receivedSignalPsd);
              NS_LOG_DEBUG ("Signal power received (watts) before antenna gain for RU with type " << ruType << " and range (" << range.first << "; " << range.second << ") -> (" << band.first << "; " << band.second <<  "): " << Integral (filteredSignal));
              double rxPowerPerBandW = Integral (filteredSignal) * DbToRatio (GetRxGain ());
              NS_LOG_DEBUG ("Signal power received after antenna gain for RU with type " << ruType << " and range (" << range.first << "; " << range.second << ") -> (" << band.first << "; " << band.second <<  "): " << rxPowerPerBandW << " W (" << WToDbm (rxPowerPerBandW) << " dBm)");
              rxPowerW.push_back (std::make_pair (band, rxPowerPerBandW));
            }
        }
    }

  NS_LOG_DEBUG ("Total signal power received after antenna gain: " << totalRxPowerW << " W (" << WToDbm (totalRxPowerW) << " dBm)");

  Ptr<WifiSpectrumSignalParameters> wifiRxParams = DynamicCast<WifiSpectrumSignalParameters> (rxParams);

  // Log the signal arrival to the trace source
  m_signalCb (wifiRxParams ? true : false, senderNodeId, WToDbm (totalRxPowerW), rxDuration);

  // Do no further processing if signal is too weak
  // Current implementation assumes constant rx power over the PPDU duration
  if (WToDbm (totalRxPowerW) < GetRxSensitivity ())
    {
      NS_LOG_INFO ("Received signal too weak to process: " << WToDbm (totalRxPowerW) << " dBm");
      return;
    }
  if (wifiRxParams == 0)
    {
      NS_LOG_INFO ("Received non Wi-Fi signal");
      m_interference.AddForeignSignal (rxDuration, rxPowerW);
      MaybeCcaBusy ();
      return;
    }
  if (wifiRxParams && m_disableWifiReception)
    {
      NS_LOG_INFO ("Received Wi-Fi signal but blocked from syncing");
      m_interference.AddForeignSignal (rxDuration, rxPowerW);
      MaybeCcaBusy ();
      return;
    }

  NS_LOG_INFO ("Received Wi-Fi signal");
  Ptr<WifiPpdu> ppdu = Copy (wifiRxParams->ppdu);
  if ((ppdu->GetTxVector ().GetPreambleType () == WIFI_PREAMBLE_HE_TB))
    {
      bool isOfdma = (rxDuration == (ppdu->GetTxDuration () - CalculatePlcpPreambleAndHeaderDuration (ppdu->GetTxVector ())));
      if ((m_currentHeTbPpduUid == ppdu->GetUid ()) && (m_currentEvent != 0))
        {
          //AP already received non-OFDMA part, handle OFDMA payload reception
          StartReceiveOfdmaPayload (ppdu, rxPowerW);
        }
      else if (isOfdma)
        {
          //PHY receives the OFDMA payload but either it is not an AP or it comes from another BSS
          NS_LOG_INFO ("Consider UL-OFDMA part of the HE TB PPDU as interference since device is not AP or does not belong to the same BSS");
          m_interference.Add (ppdu, ppdu->GetTxVector (), rxDuration, rxPowerW);
          auto it = m_currentPreambleEvents.find (ppdu->GetUid ());
          if (it != m_currentPreambleEvents.end ())
            {
              m_currentPreambleEvents.erase (it);
            }
          if (m_currentPreambleEvents.empty ())
            {
              Reset ();
            }
        }
      else
        {
          //Start receiving non-OFDMA preamble
          StartReceivePreamble (ppdu, rxPowerW);
        }
    }
  else
    {
      StartReceivePreamble (ppdu, rxPowerW);
    }
}

Ptr<AntennaModel>
SpectrumWifiPhy::GetRxAntenna (void) const
{
  return m_antenna;
}

void
SpectrumWifiPhy::SetAntenna (const Ptr<AntennaModel> a)
{
  NS_LOG_FUNCTION (this << a);
  m_antenna = a;
}

void
SpectrumWifiPhy::CreateWifiSpectrumPhyInterface (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  m_wifiSpectrumPhyInterface = CreateObject<WifiSpectrumPhyInterface> ();
  m_wifiSpectrumPhyInterface->SetSpectrumWifiPhy (this);
  m_wifiSpectrumPhyInterface->SetDevice (device);
}

Ptr<SpectrumValue>
SpectrumWifiPhy::GetTxPowerSpectralDensity (double txPowerW, Ptr<WifiPpdu> ppdu, bool isOfdma)
{
  WifiTxVector txVector = ppdu->GetTxVector ();
  uint16_t centerFrequency = GetCenterFrequencyForChannelWidth (txVector.GetChannelWidth ());
  uint16_t channelWidth = txVector.GetChannelWidth ();
  NS_LOG_FUNCTION (centerFrequency << channelWidth << txPowerW);
  Ptr<SpectrumValue> v;
  switch (ppdu->GetModulation ())
    {
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_ERP_OFDM:
      if (channelWidth >= 40)
        {
            NS_LOG_INFO ("non-HT duplicate");
            v = WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW, GetGuardBandwidth (channelWidth), m_txMaskInnerBandMinimumRejection, m_txMaskOuterBandMinimumRejection, m_txMaskOuterBandMaximumRejection);
            //TODO: Create a CreateDuplicateOfdmTxPowerSpectralDensity function?
        }
      else
        {
            v = WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW, GetGuardBandwidth (channelWidth), m_txMaskInnerBandMinimumRejection, m_txMaskOuterBandMinimumRejection, m_txMaskOuterBandMaximumRejection);
        }
      break;
    case WIFI_MOD_CLASS_DSSS:
    case WIFI_MOD_CLASS_HR_DSSS:
      NS_ABORT_MSG_IF (channelWidth != 22, "Invalid channel width for DSSS");
      v = WifiSpectrumValueHelper::CreateDsssTxPowerSpectralDensity (centerFrequency, txPowerW, GetGuardBandwidth (channelWidth));
      break;
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
      v = WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW, GetGuardBandwidth (channelWidth), m_txMaskInnerBandMinimumRejection, m_txMaskOuterBandMinimumRejection, m_txMaskOuterBandMaximumRejection);
      break;
    case WIFI_MOD_CLASS_HE:
      if (isOfdma)
        {
          WifiSpectrumBand band = GetRuBand (txVector, GetStaId (ppdu));
          v = WifiSpectrumValueHelper::CreateHeMuOfdmTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW, GetGuardBandwidth (channelWidth), band);
        }
      else
        {
          v = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW, GetGuardBandwidth (channelWidth), m_txMaskInnerBandMinimumRejection, m_txMaskOuterBandMinimumRejection, m_txMaskOuterBandMaximumRejection);
        }
      break;
    default:
      NS_FATAL_ERROR ("modulation class unknown");
      break;
    }
  return v;
}

void
SpectrumWifiPhy::StartTx (Ptr<WifiPpdu> ppdu, uint8_t txPowerLevel)
{
  NS_LOG_FUNCTION (this << ppdu << +txPowerLevel);
  WifiTxVector txVector = ppdu->GetTxVector ();
  txVector.SetTxPowerLevel (txPowerLevel);
  double txPowerDbm = GetTxPowerForTransmission (txVector) + GetTxGain ();
  NS_LOG_DEBUG ("Start transmission: signal power before antenna gain=" << txPowerDbm << "dBm");
  double txPowerWatts = DbmToW (txPowerDbm);
  NS_ASSERT_MSG (m_wifiSpectrumPhyInterface, "SpectrumPhy() is not set; maybe forgot to call CreateWifiSpectrumPhyInterface?");
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB)
    {
      //non-OFDMA part
      Time nonOfdmaDuration = CalculatePlcpPreambleAndHeaderDuration (txVector); //consider that HE-STF and HE-LTFs are also part of the non-OFDMA part
      Ptr<SpectrumValue> txPowerSpectrum = GetTxPowerSpectralDensity (txPowerWatts, ppdu);
      Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
      txParams->duration = nonOfdmaDuration;
      txParams->psd = txPowerSpectrum;
      txParams->txPhy = m_wifiSpectrumPhyInterface->GetObject<SpectrumPhy> ();
      txParams->txAntenna = m_antenna;
      txParams->ppdu = ppdu;
      NS_LOG_DEBUG ("Starting non-OFDMA transmission with power " << WToDbm (txPowerWatts) << " dBm on channel " << +GetChannelNumber () << " for " << txParams->duration.GetMicroSeconds () << " us");
      NS_LOG_DEBUG ("Starting non-OFDMA transmission with integrated spectrum power " << WToDbm (Integral (*txPowerSpectrum)) << " dBm; spectrum model Uid: " << txPowerSpectrum->GetSpectrumModel ()->GetUid ());
      Transmit (txParams);

      //OFDMA part
      Simulator::Schedule (nonOfdmaDuration, &SpectrumWifiPhy::StartOfdmaTx, this, ppdu, txPowerWatts);
    }
  else
    {
      Ptr<SpectrumValue> txPowerSpectrum = GetTxPowerSpectralDensity (txPowerWatts, ppdu);
      Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
      txParams->duration = ppdu->GetTxDuration ();
      txParams->psd = txPowerSpectrum;
      txParams->txPhy = m_wifiSpectrumPhyInterface->GetObject<SpectrumPhy> ();
      txParams->txAntenna = m_antenna;
      txParams->ppdu = ppdu;
      NS_LOG_DEBUG ("Starting transmission with power " << WToDbm (txPowerWatts) << " dBm on channel " << +GetChannelNumber () << " for " << txParams->duration.GetMicroSeconds () << " us");
      NS_LOG_DEBUG ("Starting transmission with integrated spectrum power " << WToDbm (Integral (*txPowerSpectrum)) << " dBm; spectrum model Uid: " << txPowerSpectrum->GetSpectrumModel ()->GetUid ());
      Transmit (txParams);
    }
}

void
SpectrumWifiPhy::StartOfdmaTx (Ptr<WifiPpdu> ppdu, double txPowerWatts)
{
  NS_LOG_FUNCTION (this << ppdu << txPowerWatts);
  NS_ASSERT (ppdu->IsUlMu ());
  Ptr<SpectrumValue> txPowerSpectrum = GetTxPowerSpectralDensity (txPowerWatts, ppdu, true);
  Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
  WifiTxVector txVector = ppdu->GetTxVector ();
  txParams->duration = ppdu->GetTxDuration () - CalculatePlcpPreambleAndHeaderDuration (txVector);
  txParams->psd = txPowerSpectrum;
  txParams->txPhy = m_wifiSpectrumPhyInterface->GetObject<SpectrumPhy> ();
  txParams->txAntenna = m_antenna;
  txParams->ppdu = ppdu;
  NS_LOG_DEBUG ("Starting OFDMA transmission with power " << WToDbm (txPowerWatts) << " dBm on channel " << +GetChannelNumber () << " for " << txParams->duration.GetMicroSeconds () << " us");
  NS_LOG_DEBUG ("Starting OFDMA transmission with integrated spectrum power " << WToDbm (Integral (*txPowerSpectrum)) << " dBm; spectrum model Uid: " << txPowerSpectrum->GetSpectrumModel ()->GetUid ());
  Transmit (txParams);
}

void
SpectrumWifiPhy::Transmit (Ptr<WifiSpectrumSignalParameters> txParams)
{
  NS_LOG_FUNCTION (this << txParams);
  m_channel->StartTx (txParams);
}

uint32_t
SpectrumWifiPhy::GetBandBandwidth (void) const
{
  uint32_t bandBandwidth = 0;
  switch (GetStandard ())
    {
    case WIFI_PHY_STANDARD_80211a:
    case WIFI_PHY_STANDARD_80211g:
    case WIFI_PHY_STANDARD_holland:
    case WIFI_PHY_STANDARD_80211b:
    case WIFI_PHY_STANDARD_80211n_2_4GHZ:
    case WIFI_PHY_STANDARD_80211n_5GHZ:
    case WIFI_PHY_STANDARD_80211ac:
      // Use OFDM subcarrier width of 312.5 KHz as band granularity
      bandBandwidth = 312500;
      break;
    case WIFI_PHY_STANDARD_80211_10MHZ:
      // Use OFDM subcarrier width of 156.25 KHz as band granularity
      bandBandwidth = 156250;
      break;
    case WIFI_PHY_STANDARD_80211_5MHZ:
      // Use OFDM subcarrier width of 78.125 KHz as band granularity
      bandBandwidth = 78125;
      break;
    case WIFI_PHY_STANDARD_80211ax_2_4GHZ:
    case WIFI_PHY_STANDARD_80211ax_5GHZ:
      // Use OFDM subcarrier width of 78.125 KHz as band granularity
      bandBandwidth = 78125;
      break;
    default:
      NS_FATAL_ERROR ("Standard unknown: " << GetStandard ());
      break;
    }
  return bandBandwidth;
}

uint16_t
SpectrumWifiPhy::GetGuardBandwidth (uint16_t currentChannelWidth) const
{
  uint16_t guardBandwidth = 0;
  if (currentChannelWidth == 22)
    {
      //handle case of use of legacy DSSS transmission
      guardBandwidth = 10;
    }
  else
    {
      //In order to properly model out of band transmissions for OFDM, the guard
      //band has been configured so as to expand the modeled spectrum up to the
      //outermost referenced point in "Transmit spectrum mask" sections' PSDs of
      //each PHY specification of 802.11-2016 standard. It thus ultimately corresponds
      //to the currently considered channel bandwidth (which can be different from
      //supported channel width).
      guardBandwidth = currentChannelWidth;
    }
  return guardBandwidth;
}

WifiSpectrumBand
SpectrumWifiPhy::GetBand (uint16_t bandWidth, uint8_t bandIndex)
{
  bandWidth = (bandWidth == 22) ? 20 : bandWidth;
  uint16_t channelWidth = GetChannelWidth ();
  uint32_t bandBandwidth = GetBandBandwidth ();
  size_t numBandsInChannel = static_cast<size_t> (channelWidth * 1e6 / bandBandwidth);
  size_t numBandsInBand = static_cast<size_t> (bandWidth * 1e6 / bandBandwidth);
  if (numBandsInBand % 2 == 0)
    {
      numBandsInChannel += 1; // symmetry around center frequency
    }
  size_t totalNumBands = GetRxSpectrumModel ()->GetNumBands ();
  NS_ASSERT_MSG ((numBandsInChannel % 2 == 1) && (totalNumBands % 2 == 1), "Should have odd number of bands");
  NS_ASSERT_MSG ((bandIndex * bandWidth) < channelWidth, "Band index is out of bound");
  WifiSpectrumBand band;
  band.first = ((totalNumBands - numBandsInChannel) / 2) + (bandIndex * numBandsInBand);
  if (band.first >= totalNumBands / 2)
    {
      //step past DC
      band.first += 1;
    }
  band.second = band.first + numBandsInBand - 1;
  return band;
}

WifiSpectrumBand
SpectrumWifiPhy::ConvertHeRuSubcarriers (uint16_t channelWidth, HeRu::SubcarrierRange range) const
{
  WifiSpectrumBand convertedSubcarriers;
  uint32_t nGuardBands = static_cast<uint32_t> (((2 * GetGuardBandwidth (channelWidth) * 1e6) / GetBandBandwidth ()) + 0.5);
  uint32_t centerFrequencyIndex = 0;
  switch (channelWidth)
    {
    case 20:
      centerFrequencyIndex = (nGuardBands / 2) + 6 + 122;
      break;
    case 40:
      centerFrequencyIndex = (nGuardBands / 2) + 12 + 244;
      break;
    case 80:
      centerFrequencyIndex = (nGuardBands / 2) + 12 + 500;
      break;
    case 160:
      centerFrequencyIndex = (nGuardBands / 2) + 12 + 1012;
      break;
    default:
      NS_FATAL_ERROR ("ChannelWidth " << channelWidth << " unsupported");
      break;
    }
  convertedSubcarriers.first = centerFrequencyIndex + range.first;
  convertedSubcarriers.second = centerFrequencyIndex + range.second;
  return convertedSubcarriers;
}

} //namespace ns3
