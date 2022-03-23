#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/bridge-module.h"
#include "ns3/yans-wifi-phy.h"
#include "ns3/wifi-net-device.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MultiApWifiScript");

// static void PrintPositions(std::string s, NodeContainer &staNodes)
// {
//     std::cout << "t= " << Simulator::Now().GetSeconds() << " " << s << std::endl;
//     Ptr<MobilityModel> mob = staNodes.Get(0)->GetObject<MobilityModel>();
//     Vector pos = mob->GetPosition();
//     std::cout << "x: " << pos.x << ", y: " << pos.y << std::endl;
//     Simulator::Schedule(Seconds(1), (&PrintPositions), s, staNodes);
// }

// static void SetVelocity(Ptr<Node> node, Vector velocity)
// {
//     Ptr<ConstantVelocityMobilityModel> mobility = node->GetObject<ConstantVelocityMobilityModel>();
//     mobility->SetVelocity(velocity);
// }
// static void getOperatingChannel(Ptr<NetDevice> device)
// {
//     Ptr<WifiNetDevice> staWifidc = device->GetObject<WifiNetDevice>();
//     Ptr<WifiPhy> staPhy = staWifidc->GetPhy();
//     Ptr<YansWifiPhy> yansStaPhy = staPhy->GetObject<YansWifiPhy>();
//     std::cout << "schedule Channel width: " << yansStaPhy->GetChannelWidth() << std::endl;
//     std::cout << "schedule Channel Number: " << yansStaPhy->GetChannelNumber() << std::endl;
//     std::cout << "schedule operating channel: " << std::endl;
// }
int main(int argc, char *argv[])
{

    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);

    NodeContainer csmaNodes;
    NodeContainer staNodes;

    csmaNodes.Create(5);
    staNodes.Create(2);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NodeContainer csServerNode(csmaNodes.Get(0), csmaNodes.Get(1));
    NodeContainer rsNode(csmaNodes.Get(0), csmaNodes.Get(2));
    NodeContainer csClientNode1(csmaNodes.Get(2), csmaNodes.Get(3));
    NodeContainer csClientNode2(csmaNodes.Get(2), csmaNodes.Get(4));
    NetDeviceContainer csServerDevice, rsDevice, csClientDevice1, csClientDevice2;
    csServerDevice = csma.Install(csServerNode);
    rsDevice = csma.Install(rsNode);
    csClientDevice1 = csma.Install(csClientNode1);
    csClientDevice2 = csma.Install(csClientNode2);

    NetDeviceContainer sta1Devices;
    NetDeviceContainer apDevices;
    NodeContainer apNodes(csmaNodes.Get(3), csmaNodes.Get(4));

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(15));
    phy.SetChannel(channel.Create());
    WifiHelper wifi;
    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    //wifi.SetStandard(WIFI_PHY_STANDARD_80211g);
    for (int i = 0; i < 2; i++)
    {
        phy.Set("ChannelNumber", UintegerValue(36));
        wifi.SetRemoteStationManager("ns3::IdealWifiManager");

        mac.SetType("ns3::ApWifiMac",
                    "Ssid", SsidValue(ssid),
                    "BeaconGeneration", BooleanValue(true));
        apDevices.Add(wifi.Install(phy, mac, apNodes.Get(i)));
    }
    Ptr<WifiPhy> phyAp0;
    Ptr<WifiNetDevice> apWifidc1 = apDevices.Get(1)->GetObject<WifiNetDevice>();
    //phyAp0 = apWifidc1->GetPhy();
    Ptr<WifiPhy> apPhy1 = apWifidc1->GetPhy();
    Ptr<YansWifiPhy> yansApPhy1 = apPhy1->GetObject<YansWifiPhy>();
    //UintegerValue index;
    //yansApPhy1->GetAttribute("Primary20MHzIndex", index);
    std::cout << "Channel width: " << yansApPhy1->GetChannelWidth() << std::endl;
    std::cout << "Channel number: " << yansApPhy1->GetChannelNumber() << std::endl;
    //std::cout << "Primary 20MHz index: " << index << std::endl;
    phy.Set("ChannelNumber", UintegerValue(36));

    //phy.Set("ChannelWidth", UintegerValue(80));
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(true));

    sta1Devices = wifi.Install(phy, mac, staNodes.Get(0));
    //Config::Set("/NodeList/5/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/ChannelWidth", UintegerValue(160));
    Ptr<WifiNetDevice> staWifidc = sta1Devices.Get(0)->GetObject<WifiNetDevice>();
    Ptr<WifiPhy> staPhy = staWifidc->GetPhy();
    Ptr<YansWifiPhy> yansStaPhy = staPhy->GetObject<YansWifiPhy>();
    std::cout << "Sta Channel width: " << yansStaPhy->GetChannelWidth() << std::endl;
    std::cout << "Sta Channel Number: " << yansStaPhy->GetChannelNumber() << std::endl;
    // YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    // YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    // channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(15));
    // phy.SetChannel(channel.Create());

    // WifiHelper wifi;
    // wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    // WifiMacHelper mac;
    // Ssid ssid = Ssid("ns-3-ssid");
    // mac.SetType("ns3::StaWifiMac",
    //             "Ssid", SsidValue(ssid),
    //             "ActiveProbing", BooleanValue(true));

    // NetDeviceContainer staDevices;
    // staDevices = wifi.Install(phy, mac, staNodes);

    // mac.SetType("ns3::ApWifiMac",
    //             "Ssid", SsidValue(ssid));

    // apDevices = wifi.Install(phy, mac, apNodes);

    NetDeviceContainer apBridgeDevices_1(apDevices.Get(0), csClientDevice1.Get(1));
    NetDeviceContainer apBridgeDevices_2(apDevices.Get(1), csClientDevice2.Get(1));
    NetDeviceContainer switchBridgeDevice(csClientDevice1.Get(0), csClientDevice2.Get(0));

    switchBridgeDevice.Add(rsDevice.Get(1));
    BridgeHelper bridge;
    bridge.Install(apNodes.Get(0), apBridgeDevices_1);
    bridge.Install(apNodes.Get(1), apBridgeDevices_2);
    bridge.Install(csmaNodes.Get(2), switchBridgeDevice);
    //NetDeviceContainer staBridgeDevice;
    //bridge.Install(staNodes.Get(0), sta1Devices);
    //std::cout << "staBridgeDevice: " << staBridgeDevice.GetN() << std::endl;

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(csmaNodes);
    Ptr<MobilityModel> mob0 = csmaNodes.Get(0)->GetObject<MobilityModel>();
    //find a faster way to set position, now is manually doing it;
    mob0->SetPosition(Vector(10, 5, 0));
    Ptr<MobilityModel> mob1 = csmaNodes.Get(1)->GetObject<MobilityModel>();
    mob1->SetPosition(Vector(20, 5, 0));
    Ptr<MobilityModel> mob2 = csmaNodes.Get(2)->GetObject<MobilityModel>();
    mob2->SetPosition(Vector(10, 10, 0));
    Ptr<MobilityModel> mob3 = csmaNodes.Get(3)->GetObject<MobilityModel>();
    mob3->SetPosition(Vector(2.5, 12.5, 0));
    Ptr<MobilityModel> mob4 = csmaNodes.Get(4)->GetObject<MobilityModel>();
    mob4->SetPosition(Vector(17.5, 12.5, 0));

    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(staNodes);
    Ptr<ConstantVelocityMobilityModel> staMob1 = staNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>();
    //Ptr<ConstantVelocityMobilityModel> staMob2 = staNodes.Get(1)->GetObject<ConstantVelocityMobilityModel>();
    staMob1->SetPosition(Vector(10, 17.5, 0));
    staMob1->SetVelocity(Vector(0, 0, 0));
    // Simulator::Schedule(Seconds(15), (&SetVelocity), staNodes.Get(0), Vector(0, 0, 0));
    // staMob2->SetPosition(Vector(17.5, 17.5, 0));
    // staMob2->SetVelocity(Vector(0.0, 0, 0));

    //Simulator::Schedule(Seconds(0), (&PrintPositions), "ap0", staNodes);

    InternetStackHelper stack;
    stack.SetIpv4ArpJitter(true);
    stack.Install(csServerNode);
    stack.Install(staNodes);
    stack.Install(apNodes);
    stack.Install(csClientNode1.Get(0));

    Ipv4AddressHelper address;
    address.SetBase("172.30.1.0", "255.255.255.0");
    Ipv4InterfaceContainer serverInterfaces;
    serverInterfaces = address.Assign(csServerDevice);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4> ipv4Ptr = csServerNode.Get(1)->GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting(ipv4Ptr);
    staticRoutingA->AddNetworkRouteTo(Ipv4Address("172.30.0.0"), Ipv4Mask("/24"), Ipv4Address("172.30.1.1"), 1);

    DhcpHelper dhcpHelper;

    Ipv4InterfaceContainer fixedNodes = dhcpHelper.InstallFixedAddress(rsDevice.Get(0), Ipv4Address("172.30.0.1"), Ipv4Mask("/24"));
    Ipv4InterfaceContainer fixedSta = dhcpHelper.InstallFixedAddress(sta1Devices.Get(0), Ipv4Address("172.30.0.2"), Ipv4Mask("/24"));
    //Ipv4InterfaceContainer fixedSta1 = dhcpHelper.InstallFixedAddress(sta1Devices.Get(1), Ipv4Address("172.30.0.2"), Ipv4Mask("/24"));
    ApplicationContainer dhcpServerApp = dhcpHelper.InstallDhcpServer(rsDevice.Get(0), Ipv4Address("172.30.0.1"),
                                                                      Ipv4Address("172.30.0.0"), Ipv4Mask("/24"),
                                                                      Ipv4Address("172.30.0.10"), Ipv4Address("172.30.0.15"),
                                                                      Ipv4Address("172.30.0.1"));
    dhcpServerApp.Start(Seconds(0.0));
    dhcpServerApp.Stop(Seconds(20));
    Ptr<Ipv4StaticRouting> staRouting;
    staRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(staNodes.Get(0)->GetObject<Ipv4>()->GetRoutingProtocol());
    staRouting->SetDefaultRoute("172.30.0.1", 1);
    //staRouting->AddNetworkRouteTo(Ipv4Address("172.30.0.1"), Ipv4Mask("255.255.255.0"), 2);

    //DynamicCast<DhcpServer>(dhcpServerApp.Get(0))->AddStaticDhcpEntry(staDevices.Get(0)->GetAddress(), Ipv4Address("172.30.0.14"));

    // ApplicationContainer dhcpClients = dhcpHelper.InstallDhcpClient(staDevices.Get(1));
    // dhcpClients.Start(Seconds(0.5));
    // dhcpClients.Stop(Seconds(20));

    // UdpEchoServerHelper echoServer(9);
    // ApplicationContainer serverApps = echoServer.Install(csServerNode.Get(1));
    // serverApps.Start(Seconds(2.0));
    // serverApps.Stop(Seconds(20.0));

    // UdpEchoClientHelper echoClient(serverInterfaces.GetAddress(1), 9);
    // echoClient.SetAttribute("MaxPackets", UintegerValue(10));
    // echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    // echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // ApplicationContainer clientApps = echoClient.Install(staNodes.Get(0));
    // clientApps.Start(Seconds(5.0));
    // clientApps.Stop(Seconds(7));
    // clientApps.Start(Seconds(10));
    // clientApps.Stop(Seconds(20));
    UdpEchoServerHelper echoServer1(21);
    ApplicationContainer serverApps1 = echoServer1.Install(csServerNode.Get(1));
    serverApps1.Start(Seconds(2.0));
    serverApps1.Stop(Seconds(20.0));

    UdpEchoClientHelper echoClient1(serverInterfaces.GetAddress(1), 21);
    echoClient1.SetAttribute("MaxPackets", UintegerValue(10));
    echoClient1.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient1.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps1 = echoClient1.Install(staNodes.Get(0));
    clientApps1.Start(Seconds(3.0));
    clientApps1.Stop(Seconds(7));

    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(staNodes.Get(0));
    serverApps.Start(Seconds(2.0));
    serverApps.Stop(Seconds(20.0));

    UdpEchoClientHelper echoClient(fixedSta.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(csServerNode.Get(1));
    clientApps.Start(Seconds(8.0));

    clientApps.Stop(Seconds(20));

    Ipv4GlobalRoutingHelper g;
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("routes.txt", std::ios::out);
    //g.PrintRoutingTableAllAt(Seconds(6.0), routingStream);
    //g.PrintRoutingTableAllAt(Seconds(12.0), routingStream);

    AnimationInterface anim("multi_ap_wifi_test.xml");
    //phy.EnablePcap("multi_ap_wifi_test_sta_1", staDevices.Get(1));
    //YansWifiPhyHelper phy;
    phy.EnablePcap("multi_ap_wifi_test_sta_0", sta1Devices.Get(0));
    csma.EnablePcap("multi_ap_wifi_test_server_rt", csServerDevice.Get(0));
    csma.EnablePcap("multi_ap_wifi_test_rt", rsDevice.Get(0));
    Simulator::Stop(Seconds(20));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}