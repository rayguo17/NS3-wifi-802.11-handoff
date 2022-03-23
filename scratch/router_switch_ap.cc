#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/bridge-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RouterSwitchApScript");

int main(int argc, char *argv[])
{
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    CommandLine cmd;
    cmd.Parse(argc, argv);

    NodeContainer nodeAll;
    NodeContainer staNodes;

    nodeAll.Create(4);
    staNodes.Create(1);
    NodeContainer p2pNodes(nodeAll.Get(0), nodeAll.Get(1));
    NodeContainer rsNodes(nodeAll.Get(1), nodeAll.Get(2));
    NodeContainer saNodes(nodeAll.Get(2), nodeAll.Get(3));
    NodeContainer apNodes(nodeAll.Get(3));

    PointToPointHelper p2pHelper;
    p2pHelper.SetChannelAttribute("Delay", StringValue("2ms"));
    p2pHelper.SetDeviceAttribute("DataRate", StringValue("5Mbps"));

    NetDeviceContainer p2pDevices;
    p2pDevices = p2pHelper.Install(p2pNodes);

    CsmaHelper csmaHelper;
    csmaHelper.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csmaHelper.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer rsDevices, saDevices;
    rsDevices = csmaHelper.Install(rsNodes);
    saDevices = csmaHelper.Install(saNodes);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
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
    apDevices = wifi.Install(phy, mac, apNodes);

    NetDeviceContainer switchBridgeDevices(rsDevices.Get(1), saDevices.Get(0));
    NetDeviceContainer apBridgeDevices(apDevices.Get(0), saDevices.Get(1));
    BridgeHelper bridge;
    bridge.Install(nodeAll.Get(2), switchBridgeDevices);
    bridge.Install(apNodes.Get(0), apBridgeDevices);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(10.0),
                                  "MinY", DoubleValue(10.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(3.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("ColumnFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodeAll);
    mobility.Install(staNodes);

    InternetStackHelper stack;
    stack.Install(p2pNodes);
    stack.Install(staNodes);

    Ipv4AddressHelper address;
    address.SetBase("172.30.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4> ipv4Ptr = p2pNodes.Get(0)->GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting(ipv4Ptr);
    staticRoutingA->AddNetworkRouteTo(Ipv4Address("172.30.0.0"), Ipv4Mask("/24"), Ipv4Address("172.30.1.2"), 1);

    DhcpHelper dhcpHelper;

    Ipv4InterfaceContainer fixedNodes = dhcpHelper.InstallFixedAddress(rsDevices.Get(0), Ipv4Address("172.30.0.1"), Ipv4Mask("/24"));
    ApplicationContainer dhcpServerApp = dhcpHelper.InstallDhcpServer(rsDevices.Get(0), Ipv4Address("172.30.0.1"),
                                                                      Ipv4Address("172.30.0.0"), Ipv4Mask("/24"),
                                                                      Ipv4Address("172.30.0.10"), Ipv4Address("172.30.0.15"),
                                                                      Ipv4Address("172.30.0.1"));
    dhcpServerApp.Start(Seconds(0.0));
    dhcpServerApp.Stop(Seconds(20));

    ApplicationContainer dhcpClients = dhcpHelper.InstallDhcpClient(staDevices);
    dhcpClients.Start(Seconds(0.5));
    dhcpClients.Stop(Seconds(20));

    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(p2pNodes.Get(0));
    serverApps.Start(Seconds(2.0));
    serverApps.Stop(Seconds(20.0));

    UdpEchoClientHelper echoClient(p2pInterfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(staNodes.Get(0));
    clientApps.Start(Seconds(5.0));
    clientApps.Stop(Seconds(20));

    csmaHelper.EnablePcap("router_switch_ap", rsDevices.Get(0));
    AnimationInterface anim("router_switch_ap.xml");
    Simulator::Stop(Seconds(20));
    Simulator::Run();
    Simulator::Destroy();
}