/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleApScript");

void turnMoveBack();

int main(int argc, char *argv[])
{
    //wifi longest distance is 10 , longer than that will disconnect
    //Config::SetDefault("ns3::RangePropagationLossModel::MaxRange",DoubleValue(10));
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    CommandLine cmd;
    cmd.Parse(argc, argv);

    NodeContainer nodeAll;
    nodeAll.Create(3);
    NodeContainer staNodes;
    staNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices_1, p2pDevices_2;
    p2pDevices_1 = pointToPoint.Install(nodeAll.Get(0), nodeAll.Get(1));
    p2pDevices_2 = pointToPoint.Install(nodeAll.Get(0), nodeAll.Get(2));

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(10));
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, staNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, nodeAll.Get(1));
    apDevices.Add(wifi.Install(phy, mac, nodeAll.Get(2)));

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(15.0),
                                  "MinY", DoubleValue(10.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(5.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodeAll.Get(0));
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(10.0),
                                  "MinY", DoubleValue(15.0),
                                  "DeltaX", DoubleValue(15.0),
                                  "DeltaY", DoubleValue(5.0),
                                  "GridWidth", UintegerValue(2),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.Install(nodeAll.Get(1));
    mobility.Install(nodeAll.Get(2));
    //double velocity = 2.0;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(staNodes.Get(0));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(staNodes.Get(1));
    Ptr<MobilityModel> mob1 = staNodes.Get(0)->GetObject<MobilityModel>();
    Ptr<MobilityModel> mob2 = staNodes.Get(1)->GetObject<MobilityModel>();
    // Simulator::Schedule(Seconds(8.0), turnMoveBack, mob1, -velocity);
    // Simulator::Schedule(Seconds(8.0), turnMoveBack, mob2, velocity);

    InternetStackHelper stack;
    Ipv4GlobalRoutingHelper globalRoutingHelper;
    stack.SetRoutingHelper(globalRoutingHelper);
    stack.Install(nodeAll);
    Ipv4StaticRoutingHelper staticRoutingHelper;
    stack.SetRoutingHelper(staticRoutingHelper);
    stack.Install(staNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.0.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pOneInterface;
    p2pOneInterface = address.Assign(p2pDevices_1);

    address.SetBase("10.2.0.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pTwoInterface;
    p2pTwoInterface = address.Assign(p2pDevices_2);

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apOneInterface;
    apOneInterface = address.Assign(apDevices.Get(0));
    apOneInterface.Add(address.Assign(staDevices.Get(0)));

    address.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apTwoInterface;
    apTwoInterface = address.Assign(apDevices.Get(1));
    apTwoInterface.Add(address.Assign(staDevices.Get(1)));

    Ptr<Ipv4> ip1, ip2;
    Ptr<Ipv4StaticRouting> routeSta1, routeSta2;
    ip1 = staNodes.Get(0)->GetObject<Ipv4>();
    ip2 = staNodes.Get(1)->GetObject<Ipv4>();
    routeSta1 = Ipv4StaticRoutingHelper::GetRouting<Ipv4StaticRouting>(ip1->GetRoutingProtocol());
    routeSta2 = Ipv4StaticRoutingHelper::GetRouting<Ipv4StaticRouting>(ip2->GetRoutingProtocol());
    routeSta1->SetDefaultRoute(Ipv4Address("10.1.1.1"), 1);
    routeSta2->SetDefaultRoute(Ipv4Address("10.2.1.1"), 1);

    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(staNodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    UdpClientHelper echoClient(apOneInterface.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(20));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(staNodes.Get(1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(18.0));
    Simulator::Stop(Seconds(20));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    phy.EnablePcap("sim_ap", apDevices.Get(1));
    AnimationInterface anim("sim_ap.xml");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}

void turnMoveBack(Ptr<MobilityModel> mob, double &velocity)
{

    mob->SetAttribute("Speed", Vector2DValue(Vector2D(velocity, 0)));
}