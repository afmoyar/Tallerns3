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
Las otras dos, nettwo, netone, solo se pueden comunicar através de netone
El numero minimo de nodos de cada red puede tener es de dos. En cada red habra almenos
una cabeza de cluster, que se encarga de comunicar
Parámetros de consola que pueden personalizar la ejecucion del programa
./waf --run "scratch/taller --numPackets=10" --vis
./waf --run "scratch/taller --numPackets=30 --numNodesNetTwo=20 --numNodesNetOne=15" --vis
./waf --run "scratch/taller --numPackets=30 --main_distance=10 --two_distance=10 --three_distance=10" --vis
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
#include "ns3/olsr-helper.h"
#include "ns3/ssid.h"
#include "ns3/on-off-helper.h"
#include "ns3/qos-txop.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/csma-helper.h"
#include "ns3/animation-interface.h"
#include "ns3/netanim-module.h"

#include "ns3/core-module.h"
#include "ns3/opengym-module.h"


using namespace ns3;
AnimationInterface * pAnim = 0;
//Definicion de valores por defecto de variables que se pueden modificar como parametros del
//programa
uint32_t bridge2_id;
uint32_t bridge3_id;
uint32_t recived_packages;
uint32_t packetSize; // bytes
uint32_t numPackets;
uint32_t numNodesNetOne;
uint32_t numNodesNetTwo;
uint32_t numNodesNetThree;
uint32_t source_node;
uint32_t destiny_node;
double main_x_coord;
double main_y_coord;
double two_x_coord;
double two_y_coord;
double three_x_coord;
double three_y_coord;
double main_distance;
double two_distance;
double three_distance;
double interval;


//Definición de colores para nodos
uint8_t black_r = 0;
uint8_t black_g = 0;
uint8_t black_b = 0;

uint8_t blue_r = 30;
uint8_t blue_g = 144;
uint8_t blue_b = 255;
NS_LOG_COMPONENT_DEFINE ("taller");


//Ptr<OpenGymInterface> openGymInterface =  CreateObject<OpenGymInterface> (5555);
/*
Ptr<OpenGymSpace> GetObservationSpace();
Ptr<OpenGymSpace> GetActionSpace();
Ptr<OpenGymDataContainer> GetObservation();
float GetReward();
bool GetGameOver();
std::string GetExtraInfo();
bool ExecuteActions(Ptr<OpenGymDataContainer> action);
*/


static void Simulation_Results(){
  NS_LOG_UNCOND ("********END OF SIMULATION******");
  NS_LOG_UNCOND ("Number of packages sent: "<<numPackets);
  NS_LOG_UNCOND ("Number of packages recived: "<<recived_packages);
  double percent = 100 - ((double)recived_packages/(double)numPackets)*100;
  NS_LOG_UNCOND ("Percentage of lost packages: "<<percent<<"%");

}



static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule(pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

void ReceivePacket (Ptr<Socket> socket)
{
  bool retransmit = false;
  while (socket->Recv ())
    {

      retransmit = false;
      uint32_t id_of_reciving_node = socket->GetNode()->GetId();

      NS_LOG_UNCOND (id_of_reciving_node <<" Received one packet!");
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      Ptr<Socket> source = Socket::CreateSocket (socket->GetNode(), tid);
      InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
      if(id_of_reciving_node == bridge2_id){
        NS_LOG_UNCOND ("This is cluster head for net 2");
        //retransmitir en broadcast en la red principal
        retransmit = true;
        remote = InetSocketAddress (Ipv4Address ("192.168.0.255"), 80);
        
      }else if(id_of_reciving_node == bridge3_id){
        NS_LOG_UNCOND ("This is cluster head for net 3");
        //retransmitir en broadcast en la red tres
        retransmit = true;
        remote = InetSocketAddress (Ipv4Address ("192.168.2.255"), 80);

      }else{
        NS_LOG_UNCOND ("DESTINY NODE!!!");
        recived_packages++;
      }

      if(retransmit){
        source->SetAllowBroadcast (true);
        source->Connect (remote);
        //Programando evento
        Simulator::ScheduleWithContext (id_of_reciving_node,
                                  Seconds (0.3), &GenerateTraffic,
                                  source, packetSize, 1, Seconds (interval));
      }
    }
}


//OPENAI GYM
/*
Define observation space
*/
Ptr<OpenGymSpace> MyGetObservationSpace(void)
{
  uint32_t nodeNum = NodeList::GetNNodes();
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {nodeNum,};
  std::string dtype = TypeNameGet<uint32_t> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
  NS_LOG_UNCOND ("MyGetObservationSpace: " << space);
  return space;
}

/*
Define action space
*/
Ptr<OpenGymSpace> MyGetActionSpace(void)
{
  uint32_t nodeNum = NodeList::GetNNodes();;

  Ptr<OpenGymDiscreteSpace> space = CreateObject<OpenGymDiscreteSpace> (nodeNum);
  NS_LOG_UNCOND ("MyGetActionSpace: " << space);
  return space;
}

/*
Define game over condition
*/
bool MyGetGameOver(void)
{

  bool isGameOver = false;
  bool test = false;
  static float stepCounter = 0.0;
  stepCounter += 1;
  if (stepCounter == 10 && test) {
      isGameOver = true;
  }
  NS_LOG_UNCOND ("MyGetGameOver: " << isGameOver);
  return isGameOver;
}

/*
Collect observations
*/
Ptr<OpenGymDataContainer> MyGetObservation(void)
{
  uint32_t nodeNum = NodeList::GetNNodes();;
  uint32_t low = 0.0;
  uint32_t high = 10.0;
  Ptr<UniformRandomVariable> rngInt = CreateObject<UniformRandomVariable> ();

  std::vector<uint32_t> shape = {nodeNum,};
  Ptr<OpenGymBoxContainer<uint32_t> > box = CreateObject<OpenGymBoxContainer<uint32_t> >(shape);

  // generate random data
  for (uint32_t i = 0; i<nodeNum; i++){
    uint32_t value = rngInt->GetInteger(low, high);
    box->AddValue(value);
  }

  NS_LOG_UNCOND ("MyGetObservation: " << box);
  return box;
}

/*
Define reward function
*/
float MyGetReward(void)
{
  static float reward = 0.0;
  reward += 1;
  return reward;
}

/*
Define extra info. Optional
*/
std::string MyGetExtraInfo(void)
{
  std::string myInfo = "extraInfo";
  myInfo += "n";
  NS_LOG_UNCOND("MyGetExtraInfo: " << myInfo);
  return myInfo;
}


/*
Execute received actions
*/
bool MyExecuteActions(Ptr<OpenGymDataContainer> action)
{
  Ptr<OpenGymDiscreteContainer> discrete = DynamicCast<OpenGymDiscreteContainer>(action);
  NS_LOG_UNCOND ("MyExecuteActions: " << action);
  return true;
}

void ScheduleNextStateRead(double envStepTime, Ptr<OpenGymInterface> openGym)
{
  Simulator::Schedule (Seconds(envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
  openGym->NotifyCurrentState();
}


//END OPENAI GYM


int main (int argc, char *argv[])
{
    //DEfinicion de sistema de logs que se usara
    NS_LOG_INFO("CREATING NODES.");
    LogComponentEnable("taller", LOG_LEVEL_INFO);

    //Definicion de valores por defecto de variables que se pueden modificar como parametros del
    //programa
    packetSize = 1000; // bytes
    numPackets = 4;
    numNodesNetOne = 10;
    numNodesNetTwo = 10;
    numNodesNetThree = 10;
    interval = 1.0; // seconds
    main_x_coord = 70.0;
    main_y_coord = 20.0;
    main_distance = 10.0;
    two_x_coord = 120.0;
    two_y_coord = 50.0;
    two_distance = 20.0;
    three_x_coord = 0.0;
    three_y_coord = 50.0;
    three_distance = 20.0;
    source_node = 2;
    destiny_node = 3;

    uint32_t stopTime = numPackets*interval+1;

    
    //Configuracion de parametros de programa que se podran ingresar mediante ./waf ...taller --<par> = valor
    CommandLine cmd;
    cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue ("numPackets", "number of packets generated", numPackets);
    cmd.AddValue ("numNodesNetOne", "number of nodes for main network", numNodesNetOne);
    cmd.AddValue ("numNodesNetTwo", "number of nodes for second network", numNodesNetTwo);
    cmd.AddValue ("numNodesNetThree", "number of nodes for third network", numNodesNetThree);
    cmd.AddValue ("source_node", "node from net two that sends traffic", source_node);
    cmd.AddValue ("destiny_node", "node from net three that receives traffic", destiny_node);
    cmd.AddValue ("interval", "seconds betwen one package and another", interval);
    cmd.AddValue ("main_x_coord", "X coordinate of first node of main net", main_x_coord);
    cmd.AddValue ("main_y_coord", "Y coordinate of first node of main net", main_y_coord);
    cmd.AddValue ("main_distance", "distances that separates nodes in main net", main_distance);
    cmd.AddValue ("two_x_coord", "X coordinate of first node of second net", two_x_coord);
    cmd.AddValue ("two_y_coord", "Y coordinate of first node of second net", two_y_coord);
    cmd.AddValue ("two_distance", "distances that separates nodes in second net", two_distance);
    cmd.AddValue ("three_x_coord", "X coordinate of first node of third net", three_x_coord);
    cmd.AddValue ("three_y_coord", "Y coordinate of first node of third net", three_y_coord);
    cmd.AddValue ("three_distance", "distances that separates nodes in third net", three_distance);
    cmd.Parse (argc, argv);

    //si el usuario escoge un numero de nodos menor al limite, automanticamente este valor se cambia
    //por el valor minimo
    uint32_t minValue = 2;
    if(numNodesNetOne < 2) numNodesNetOne = minValue;
    if(numNodesNetTwo < 2) numNodesNetTwo = minValue;
    if(numNodesNetThree < 2) numNodesNetThree = minValue;
    //se establecen los nodos que van a servir como conexion entre las redes de baja jerarquia
    // y la red principal
    uint32_t mainTwoAttachmentIndex = numNodesNetOne-1;
    uint32_t mainThreeAttachmentIndex = 1;


    Time interPacketInterval = Seconds (interval);

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

  //bridge2_id es el id del nodo cabeza de cluster que comunica main con two
  bridge2_id = mainNodeContainer.Get(mainTwoAttachmentIndex)->GetId();
  NS_LOG_INFO("MAIN: cluster head between one and two id:"<< bridge2_id);

  //bridge3_id es el id del nodo cabeza de cluster que comunica main con three
  bridge3_id = mainNodeContainer.Get(mainThreeAttachmentIndex)->GetId();
  NS_LOG_INFO("MAIN: cluster head between one and two id:"<< bridge3_id);
  

  //Intalacion de dispositivos de red wifi en modo adhoc, capa fisica y capa mac a los nodos
  WifiHelper wifi;
  WifiMacHelper mac;
  mac.SetType ("ns3::AdhocWifiMac");
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate54Mbps"));
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  NetDeviceContainer mainDeviceContainer = wifi.Install (wifiPhy, mac, mainNodeContainer);


  //Intalacion de pila de protocolos a los dispositivos de red
  //NS_LOG_INFO ("MAIN:Enabling OLSR routing on all nodes");
  OlsrHelper olsr;
  InternetStackHelper internet;
  //internet.SetRoutingHelper (olsr);
  //se establace al protocolo olsr como protocolo de enrutamiento para la red.
  //Este funciona con tablas de enrutamiento en redes adhoc moviles
  internet.Install (mainNodeContainer);

  //Asignacion de direcciones ip
  NS_LOG_INFO ("MAIN:setting up ip adresses on all nodes");
  Ipv4AddressHelper ipAddrs;
  ipAddrs.SetBase ("192.168.0.0", "255.255.255.0");
  //cada dispositivo de red tendra direcciones 192.168.0.1, 192.168.0.2....192.168.0.254
  ipAddrs.Assign (mainDeviceContainer);

  //La red adhoc es movil, por lo tanto se definen atributos de movilidad a los nodos

  MobilityHelper mobility;
  //Primero se define la posicion de cada nodo en una grilla
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (main_x_coord),//Cordenada inicial en x
                                 "MinY", DoubleValue (main_y_coord),//Cordenada inicial en Y
                                 "DeltaX", DoubleValue (main_distance),//Distancia horizontal entre nodos
                                 "DeltaY", DoubleValue (main_distance),//Distancia vertical entre nodos
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  //Luego se les indica a los nodos en que forma se pueden mover
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (main_x_coord-10, main_x_coord+100, main_y_coord-10, main_y_coord+100)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  mobility.Install (mainNodeContainer);


  /******Transmision de informacion de un nodo a otro*****/
  /*
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (mainNodeContainer.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
  Ptr<Socket> source = Socket::CreateSocket (mainNodeContainer.Get (1), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);
  // Tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", mainDeviceContainer);
  // Output what we are doing
  NS_LOG_INFO ("Testing " << numPackets  << " packets sent");
  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval);
*/
/******Transmision de informacion de un nodo a otro*****/



    ///////////////////////////////////////////////////////////////////////////
  //                                                                         //
  //                                RED 2                                   //
  //                                                                       //
  ///////////////////////////////////////////////////////////////////////////

  NS_LOG_INFO ("TWO:Creating net 2 atached by main net by node " << mainTwoAttachmentIndex);
  //Creamos un contenedor temporal donde se guardan los nuevos nodos 
  NodeContainer newNodesNetTwo;
  //Creamos numNodesNetTwo - 1 nuevos nodos, el -1 es porque habrá un nodo adicional que es el de
  //union con la red principal
  newNodesNetTwo.Create (numNodesNetTwo - 1);
  // Creacion del contenedor de nodos para la red
  NodeContainer secondNodeContainer (mainNodeContainer.Get (mainTwoAttachmentIndex), newNodesNetTwo);
  NS_LOG_INFO("TWO: "<< secondNodeContainer.GetN() <<" nodes created");

  //Intalacion de dispositivos de red wifi en modo adhoc, capa fisica y capa mac a los nodos
  WifiHelper wifi2;
  WifiMacHelper mac2;
  mac2.SetType ("ns3::AdhocWifiMac");
  wifi2.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate54Mbps"));
  YansWifiPhyHelper wifiPhy2 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel2 = YansWifiChannelHelper::Default ();
  wifiPhy2.SetChannel (wifiChannel2.Create ());
  NetDeviceContainer secondDeviceContainer = wifi2.Install (wifiPhy2, mac2, newNodesNetTwo);
  secondDeviceContainer.Add(wifi2.Install (wifiPhy2, mac2, mainNodeContainer.Get(mainTwoAttachmentIndex)));
  


  //Intalacion de pila de protocolos a los dispositivos de red
  //NS_LOG_INFO ("TWO:Enabling OLSR routing on all nodes");
  OlsrHelper olsr2;
  InternetStackHelper internet2;
  //internet2.SetRoutingHelper (olsr2);
  //se establace al protocolo olsr como protocolo de enrutamiento para la red.
  //Este funciona con tablas de enrutamiento en redes adhoc moviles
  internet2.Install (newNodesNetTwo);

  //Asignacion de direcciones ip
  NS_LOG_INFO ("MAIN:setting up ip adresses on all nodes");
  Ipv4AddressHelper ipAddrs2;
  ipAddrs2.SetBase ("192.168.1.0", "255.255.255.0");
  //cada dispositivo de red tendra direcciones 192.168.1.1, 192.168.1.2....192.168.1.254
  ipAddrs2.Assign (secondDeviceContainer);

  //La red adhoc es movil, por lo tanto se definen atributos de movilidad a los nodos
  MobilityHelper mobilityB;
  //Primero se define la posicion de cada nodo en una grilla
  mobilityB.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (two_x_coord),
                                 "MinY", DoubleValue (two_y_coord),
                                 "DeltaX", DoubleValue (two_distance),
                                 "DeltaY", DoubleValue (two_distance),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  //Luego se les indica a los nodos en que forma se pueden mover
  mobilityB.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (two_x_coord-10, two_x_coord+100, two_y_coord-10, two_y_coord+100)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  mobilityB.Install(newNodesNetTwo);

  MobilityHelper mobilityTwo;
  Ptr<ListPositionAllocator> subnetAlloc =
  CreateObject<ListPositionAllocator> ();
  for (uint32_t j = 0; j < newNodesNetTwo.GetN (); ++j)
  {

        subnetAlloc->Add (Vector (0.0, j, 0.0));
  }
  mobilityTwo.PushReferenceMobilityModel (mainNodeContainer.Get (mainTwoAttachmentIndex));
  mobilityTwo.SetPositionAllocator (subnetAlloc);



  ///////////////////////////////////////////////////////////////////////////
  //                                                                         //
  //                                RED 3                                   //
  //                                                                       //
  ///////////////////////////////////////////////////////////////////////////

  NS_LOG_INFO ("THREE:Creating net 3 atached by main net by node " << mainThreeAttachmentIndex);
  //Creamos un contenedor temporal donde se guardan los nuevos nodos 
  NodeContainer newNodesNetThree;
  //Creamos newNodesNetThree - 1 nuevos nodos, el -1 es porque habrá un nodo adicional que es el de
  //union con la red principal
  newNodesNetThree.Create (numNodesNetThree - 1);
  // Creacion del contenedor de nodos para la red
  NodeContainer thirdNodeContainer (mainNodeContainer.Get (mainThreeAttachmentIndex), newNodesNetThree);
  NS_LOG_INFO("THREE: "<< thirdNodeContainer.GetN() <<" nodes created");

  //Intalacion de dispositivos de red wifi en modo adhoc, capa fisica y capa mac a los nodos
  WifiHelper wifi3;
  WifiMacHelper mac3;
  mac3.SetType ("ns3::AdhocWifiMac");
  wifi3.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate54Mbps"));
  YansWifiPhyHelper wifiPhy3 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel3 = YansWifiChannelHelper::Default ();
  wifiPhy3.SetChannel (wifiChannel3.Create ());
  NetDeviceContainer thirdDeviceContainer = wifi3.Install (wifiPhy3, mac3, newNodesNetThree);
  thirdDeviceContainer.Add(wifi3.Install (wifiPhy3, mac3, mainNodeContainer.Get(mainThreeAttachmentIndex)));
  


  //Intalacion de pila de protocolos a los dispositivos de red
  //NS_LOG_INFO ("TWO:Enabling OLSR routing on all nodes");
  OlsrHelper olsr3;
  InternetStackHelper internet3;
  //internet2.SetRoutingHelper (olsr2);
  //se establace al protocolo olsr como protocolo de enrutamiento para la red.
  //Este funciona con tablas de enrutamiento en redes adhoc moviles
  internet3.Install (newNodesNetThree);

  //Asignacion de direcciones ip
  NS_LOG_INFO ("MAIN:setting up ip adresses on all nodes");
  Ipv4AddressHelper ipAddrs3;
  ipAddrs3.SetBase ("192.168.2.0", "255.255.255.0");
  //cada dispositivo de red tendra direcciones 192.168.1.1, 192.168.1.2....192.168.1.254
  ipAddrs3.Assign (thirdDeviceContainer);

  //La red adhoc es movil, por lo tanto se definen atributos de movilidad a los nodos
  MobilityHelper mobilityC;
  //Primero se define la posicion de cada nodo en una grilla
  mobilityC.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (three_x_coord),
                                 "MinY", DoubleValue (three_y_coord),
                                 "DeltaX", DoubleValue (three_distance),
                                 "DeltaY", DoubleValue (three_distance),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  //Luego se les indica a los nodos en que forma se pueden mover
  mobilityC.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (three_x_coord-10, three_x_coord+100, three_y_coord-10, three_y_coord+100)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  mobilityC.Install(newNodesNetThree);

  MobilityHelper mobilityThree;
  Ptr<ListPositionAllocator> subnetAllocC =
  CreateObject<ListPositionAllocator> ();
  for (uint32_t j = 0; j < newNodesNetThree.GetN (); ++j)
  {

        subnetAllocC->Add (Vector (0.0, j, 0.0));
  }
  mobilityThree.PushReferenceMobilityModel (mainNodeContainer.Get (mainThreeAttachmentIndex));
  mobilityThree.SetPositionAllocator (subnetAllocC);
  


  
  
  /**************comunicacion entre clusters****************************/
  
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  //UN nodo de la red dos va a estar escuchando en el puerto 80
  Ptr<Socket> destiny = Socket::CreateSocket (thirdNodeContainer.Get (destiny_node), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  destiny->Bind (local);
  destiny->SetRecvCallback (MakeCallback (&ReceivePacket));

  //La cabeza de cluster entre red 1 y 2 también va a estar escuchando.
  Ptr<Socket> bridge_socket_2 = Socket::CreateSocket (mainNodeContainer.Get (mainTwoAttachmentIndex), tid);
  InetSocketAddress bridge_addr_2 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  bridge_socket_2->Bind (bridge_addr_2);
  bridge_socket_2->SetRecvCallback (MakeCallback (&ReceivePacket));


  //La cabeza de cluster entre red 1 y 3 también va a estar escuchando.
  Ptr<Socket> bridge_socket_3 = Socket::CreateSocket (mainNodeContainer.Get (mainThreeAttachmentIndex), tid);
  InetSocketAddress bridge_addr_3 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  bridge_socket_3->Bind (bridge_addr_3);
  bridge_socket_3->SetRecvCallback (MakeCallback (&ReceivePacket));

  //El nodo que origina el trafico esta en la red 1.
  Ptr<Socket> source = Socket::CreateSocket (secondNodeContainer.Get (source_node), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("192.168.1.255"), 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  

  // Output what we are doing
  NS_LOG_INFO ("Testing " << numPackets  << " packets sent");

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval);
                                 
  

/**************comunicacion entre clusters****************************/



  // Tracing
  wifiPhy.EnablePcap ("taller", mainDeviceContainer);

  //Configuracion de visualizacion
  
  pAnim = new AnimationInterface("animfile.xml");
  
  
  //openai gym
  // Parameters of the scenario
  uint32_t simSeed = 1;
  double simulationTime = 1; //seconds
  double envStepTime = 0.1; //seconds, ns3gym env step time interval
  uint32_t openGymPort = 5555;
  uint32_t testArg = 0;

 

  NS_LOG_UNCOND("Ns3Env parameters:");
  NS_LOG_UNCOND("--simulationTime: " << simulationTime);
  NS_LOG_UNCOND("--openGymPort: " << openGymPort);
  NS_LOG_UNCOND("--envStepTime: " << envStepTime);
  NS_LOG_UNCOND("--seed: " << simSeed);
  NS_LOG_UNCOND("--testArg: " << testArg);

  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (simSeed);

  // OpenGym Env
  Ptr<OpenGymInterface> openGym = CreateObject<OpenGymInterface> (openGymPort);
  openGym->SetGetActionSpaceCb( MakeCallback (&MyGetActionSpace) );
  openGym->SetGetObservationSpaceCb( MakeCallback (&MyGetObservationSpace) );
  openGym->SetGetGameOverCb( MakeCallback (&MyGetGameOver) );
  openGym->SetGetObservationCb( MakeCallback (&MyGetObservation) );
  openGym->SetGetRewardCb( MakeCallback (&MyGetReward) );
  openGym->SetGetExtraInfoCb( MakeCallback (&MyGetExtraInfo) );
  openGym->SetExecuteActionsCb( MakeCallback (&MyExecuteActions) );
  //Simulator::Schedule (Seconds(0.0), &ScheduleNextStateRead, envStepTime, openGym);
  //end openai gym
  
  Ptr<Node> id_source = secondNodeContainer.Get (source_node);
  Ptr<Node> id_destiny = thirdNodeContainer.Get (destiny_node);
  //Simulator::Schedule(Seconds(0.0),&SetColors,id_source,id_destiny);
  //Pogramacion de inicio y final de la simulación
  Simulator::Schedule (Seconds(0.0), &ScheduleNextStateRead, envStepTime, openGym);
  Simulator::Schedule(Seconds(stopTime-0.1),&Simulation_Results);
  Simulator::Stop (Seconds (stopTime));
    
  Simulator::Run ();
  Simulator::Destroy ();


  return 0;
  }
