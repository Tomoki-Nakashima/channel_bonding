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
 * Ported from yans-wifi-phy.h by several contributors starting
 * with Nicola Baldo and Dean Armstrong
 */

#ifndef SPECTRUM_WIFI_PHY_H
#define SPECTRUM_WIFI_PHY_H

#include "ns3/antenna-model.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-model.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "wifi-phy.h"

namespace ns3 {

struct WifiSpectrumSignalParameters;

class WifiSpectrumPhyInterface;
class WifiPpdu;

/**
 * \brief 802.11 PHY layer model
 * \ingroup wifi
 *
 * This PHY implements a spectrum-aware enhancement of the 802.11 SpectrumWifiPhy
 * model.
 *
 * This PHY model depends on a channel loss and delay
 * model as provided by the ns3::SpectrumPropagationLossModel
 * and ns3::PropagationDelayModel classes.
 *
 */
class SpectrumWifiPhy : public WifiPhy
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  SpectrumWifiPhy ();
  virtual ~SpectrumWifiPhy ();

  /**
   * Set the SpectrumChannel this SpectrumWifiPhy is to be connected to.
   *
   * \param channel the SpectrumChannel this SpectrumWifiPhy is to be connected to
   */
  void SetChannel (const Ptr<SpectrumChannel> channel);

  /**
   * Input method for delivering a signal from the spectrum channel
   * and low-level Phy interface to this SpectrumWifiPhy instance.
   *
   * \param rxParams Input signal parameters
   */
  void StartRx (Ptr<SpectrumSignalParameters> rxParams);

  /**
   * \param ppdu the PPDU to send
   * \param txPowerLevel the power level to use
   *
   * Note that now that the content of the TXVECTOR is stored in the WifiPpdu through PHY headers,
   * the calling method has to specify the TX power level to use upon transmission.
   * Indeed the TXVECTOR obtained from WifiPpdu does not have this information set.
   */
  void StartTx (Ptr<WifiPpdu> ppdu, uint8_t txPowerLevel);

  /**
   * Method to encapsulate the creation of the WifiSpectrumPhyInterface
   * object (used to bind the WifiSpectrumPhy to a SpectrumChannel) and
   * to link it to this SpectrumWifiPhy instance
   *
   * \param device pointer to the NetDevice object including this new object
   */
  void CreateWifiSpectrumPhyInterface (Ptr<NetDevice> device);
  /**
   * \param antenna an AntennaModel to include in the transmitted
   * SpectrumSignalParameters (in case any objects downstream of the
   * SpectrumWifiPhy wish to adjust signal properties based on the
   * transmitted antenna model.  This antenna is also used when
   * the underlying WifiSpectrumPhyInterface::GetRxAntenna() method
   * is called.
   *
   * Note:  this method may be split into separate SetTx and SetRx
   * methods in the future if the modelling need for this arises
   */
  void SetAntenna (const Ptr<AntennaModel> antenna);
  /**
   * Get the antenna model used for reception
   *
   * \return the AntennaModel used for reception
   */
  Ptr<AntennaModel> GetRxAntenna (void) const;
  /**
   * \return returns the SpectrumModel that this SpectrumPhy expects to be used
   * for all SpectrumValues that are passed to StartRx. If 0 is
   * returned, it means that any model will be accepted.
   */
  Ptr<const SpectrumModel> GetRxSpectrumModel ();

  /**
   * \return the width of each band (Hz)
   */
  uint32_t GetBandBandwidth (void) const;

  /**
   * \param currentChannelWidth channel width of the current transmission (MHz)
   * \return the width of the guard band (MHz)
   *
   * Note: in order to properly model out of band transmissions for OFDM, the guard
   * band has been configured so as to expand the modeled spectrum up to the
   * outermost referenced point in "Transmit spectrum mask" sections' PSDs of
   * each PHY specification of 802.11-2016 standard. It thus ultimately corresponds
   * to the current channel bandwidth (which can be different from devices max
   * channel width).
   */
  uint16_t GetGuardBandwidth (uint16_t currentChannelWidth) const;

  /**
   * Callback invoked when the Phy model starts to process a signal
   *
   * \param signalType Whether signal is WiFi (true) or foreign (false)
   * \param senderNodeId Node Id of the sender of the signal
   * \param rxPower received signal power (dBm)
   * \param duration Signal duration
   */
  typedef void (* SignalArrivalCallback) (bool signalType, uint32_t senderNodeId, double rxPower, Time duration);

  Ptr<Channel> GetChannel (void) const;

  // The following four methods call to the base WifiPhy class method
  // but also generate a new SpectrumModel if called during runtime

  virtual void SetChannelNumber (uint8_t id);

  virtual void SetFrequency (uint16_t freq);

  virtual void SetChannelWidth (uint16_t channelwidth);

  virtual void ConfigureStandard (WifiPhyStandard standard);


protected:
  // Inherited
  void DoDispose (void);
  void DoInitialize (void);

  /**
   * Get the start band index and the stop band index for a given band
   *
   * \param bandWidth the width of the band to be returned (MHz)
   * \param bandIndex the index of the band to be returned
   *
   * \return a pair of start and stop indexes that defines the band
   */
  WifiSpectrumBand GetBand (uint16_t bandWidth, uint8_t bandIndex = 0);

  /**
   * Perform run-time spectrum model change
   */
  virtual void ResetSpectrumModel (void);

  /**
   * This function is called to update the bands handled by the InterferenceHelper.
   */
  void UpdateInterferenceHelperBands (void);

  mutable Ptr<const SpectrumModel> m_rxSpectrumModel;       //!< receive spectrum model


private:
  /**
   * \param txPowerW power in W to spread across the bands
   * \param ppdu the PPDU that will be transmitted
   * \param isOfdma flag whether the constructed Tx PSD is for the OFDMA part of an HE TB PPDU
   * \return Ptr to SpectrumValue
   *
   * This is a helper function to create the right Tx PSD corresponding
   * to the standard in use.
   */
  Ptr<SpectrumValue> GetTxPowerSpectralDensity (double txPowerW, Ptr<WifiPpdu> ppdu, bool isOfdma = false);

  /**
   * \param channelWidth the total channel width (MHz) used for the OFDMA transmission
   * \param range the subcarrier range of the HE RU
   * \return the converted subcarriers
   *
   * This is a helper function to convert HE RU subcarriers, which are relative to the center frequency subcarrier, to the indexes used by the Spectrum model.
   */
  WifiSpectrumBand ConvertHeRuSubcarriers (uint16_t channelWidth, HeRu::SubcarrierRange range) const;

  /**
   * This function is called to send the OFDMA part of a PPDU.
   *
   * \param ppdu the PPDU to send
   * \param txPowerWatts the transmit power in watts
   */
  void StartOfdmaTx (Ptr<WifiPpdu> ppdu, double txPowerWatts);

  /**
   * This function is sending the signal to the Spectrum channel
   *
   * \param txParams the parameters to be provided to the Spectrum channel
   */
  void Transmit (Ptr<WifiSpectrumSignalParameters> txParams);

  Ptr<SpectrumChannel> m_channel; //!< SpectrumChannel that this SpectrumWifiPhy is connected to

  Ptr<WifiSpectrumPhyInterface> m_wifiSpectrumPhyInterface; //!< Spectrum phy interface
  Ptr<AntennaModel> m_antenna;                              //!< antenna model
  bool m_disableWifiReception;                              //!< forces this Phy to fail to sync on any signal
  TracedCallback<bool, uint32_t, double, Time> m_signalCb;  //!< Signal callback

  double m_txMaskInnerBandMinimumRejection; //!< The minimum rejection (in dBr) for the inner band of the transmit spectrum mask
  double m_txMaskOuterBandMinimumRejection; //!< The minimum rejection (in dBr) for the outer band of the transmit spectrum mask
  double m_txMaskOuterBandMaximumRejection; //!< The maximum rejection (in dBr) for the outer band of the transmit spectrum mask
};

} //namespace ns3

#endif /* SPECTRUM_WIFI_PHY_H */
