/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 */

/*
En este codigo se definen 3 redes adhoc inalambricas
La red de mayor jerarquia sera netone
Las otras dos, nettwo, netone, solo se podrían comunicar através de netone

*/




#include <iostream>
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("taller");


int main (int argc, char *argv[])
{
    //DEfinicion de sistema de logs que se usara
    NS_LOG_INFO("CREATING NODES.");
    LogComponentEnable("taller", LOG_LEVEL_INFO);

    //Definicion de valores por defecto de variables que se pueden modificar como parametros del
    //programa
    uint32_t packetSize = 1000; // bytes
    uint32_t numPackets = 1;
    uint32_t numNodesNetOne = 10;
    uint32_t numNodesNetTwo = 10;
    uint32_t numNodesNetThree = 10;

    //Configuracion de parametros de programa que se podran ingresar mediante ./waf ...taller --<par> = valor
    CommandLine cmd;
    cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue ("numPackets", "number of packets generated", numPackets);
    cmd.AddValue ("numNodesNetOne", "number of nodes for main network", numNodesNetOne);
    cmd.AddValue ("numNodesNetTwo", "number of nodes for second network", numNodesNetTwo);
    cmd.AddValue ("numNodesNetThree", "number of nodes for third network", numNodesNetThree);
    cmd.Parse (argc, argv);

    ///////////////////////////////////////////////////////////////////////////
  //                                                                         //
  //                            RED PRINCIPAL                               //
  //                                                                       //
  ///////////////////////////////////////////////////////////////////////////

  //Contenedor de nodos que almacena todos los nodos de la red principal
  NodeContainer mainNodeContainer;
  //Creacion de los nodos
  mainNodeContainer.Create(numNodesNetOne);
  NS_LOG_INFO("MAIN: "<< numNodesNetOne <<" nodes created");





  

  return 0;
}

