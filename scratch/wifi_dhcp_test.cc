#include "ns3/point-to-point-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WifiDhcpTestScript");

int main(int argc, char *argv[])
{
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    CommandLine cmd;
    cmd.Parse(argc, argv);

    NodeContainer p2pNodes;
    NodeContainer staNodes;
    p2pNodes.Create(2);
    staNodes.Create(3);

    PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2pHelper.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = p2pHelper.Install(p2pNodes);

    NodeContainer apNode;
    apNode = p2pNodes.Get(1);

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
    NetDeviceContainer apDevice;
    apDevice = wifi.Install(phy, mac, apNode);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(10.0),
                                  "MinY", DoubleValue(10.0),
                                  "DeltaX", DoubleValue(3.0),
                                  "DeltaY", DoubleValue(3.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(staNodes);
    mobility.Install(p2pNodes);

    InternetStackHelper stack;
    stack.Install(p2pNodes);
    stack.Install(staNodes);

    Ipv4AddressHelper address;
    address.SetBase("172.30.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    Ptr<MobilityModel> mob1 = staNodes.Get(0)->GetObject<MobilityModel>();
    mob1->SetPosition(Vector(13, 2, 0));

    // address.SetBase("172.30.0.0", "255.255.255.0");
    // Ipv4InterfaceContainer wirelessInterfaces;
    // wirelessInterfaces = address.Assign(apDevice);
    // wirelessInterfaces.Add(address.Assign(staDevices));

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4> ipv4Ptr = p2pNodes.Get(0)->GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting(ipv4Ptr);
    staticRoutingA->AddNetworkRouteTo(Ipv4Address("172.30.0.0"), Ipv4Mask("/24"), Ipv4Address("172.30.1.2"), 1);

    NS_LOG_INFO("Setup the IP addresses and create DHCP applications.");
    DhcpHelper dhcpHelper;

    Ipv4InterfaceContainer fixedNodes = dhcpHelper.InstallFixedAddress(apDevice.Get(0), Ipv4Address("172.30.0.1"), Ipv4Mask("/24"));

    ApplicationContainer dhcpServerApp = dhcpHelper.InstallDhcpServer(apDevice.Get(0), Ipv4Address("172.30.0.1"),
                                                                      Ipv4Address("172.30.0.0"), Ipv4Mask("/24"),
                                                                      Ipv4Address("172.30.0.10"), Ipv4Address("172.30.0.15"),
                                                                      Ipv4Address("172.30.0.1"));
    dhcpServerApp.Start(Seconds(0.0));
    dhcpServerApp.Stop(Seconds(20));

    //DHCP clients
    ApplicationContainer dhcpClients = dhcpHelper.InstallDhcpClient(staDevices);
    dhcpClients.Start(Seconds(0.5));
    dhcpClients.Stop(Seconds(20.0));
    // Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(p2pNodes.Get(0));
    serverApps.Start(Seconds(2.0));
    serverApps.Stop(Seconds(20));

    UdpEchoClientHelper echoClient(p2pInterfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(staNodes.Get(2));
    clientApps.Start(Seconds(5.0));
    clientApps.Stop(Seconds(20));

    phy.EnablePcap("wifi_dhcp_test", apDevice.Get(0));
    AnimationInterface anim("wifi_dhcp_test.xml");
    Simulator::Stop(Seconds(25));
    NS_LOG_INFO("Run Simulation");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
}