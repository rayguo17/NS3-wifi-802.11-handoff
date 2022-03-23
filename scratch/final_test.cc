#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/bridge-module.h"
#include "ns3/rip-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/internet-apps-module.h"

using namespace ns3;

Time stopTime = Seconds(100);
uint32_t segmentSize = 524;

NS_LOG_COMPONENT_DEFINE("FinalTestScript");

int main(int argc, char **argv)
{

    //int ftpSinkPort = 21;
    //int webSinkPort = 80;
    std::string socketFactory = "ns3::TcpSocketFactory";
    std::string tcpTypeId = "ns3::TcpNewReno";
    std::string qdiscTypeId = "ns3::FifoQueueDisc";
    bool isSack = true;
    uint32_t delAckCount = 1;
    std::string recovery = "ns3::TcpClassicRecovery";
    //Tcp设置，依次为重传机制，使用的tcp版本（此处为new reno），发送缓冲区，接收缓冲区
    Config::SetDefault("ns3::TcpL4Protocol::RecoveryType", TypeIdValue(TypeId::LookupByName(recovery)));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName(tcpTypeId)));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 20));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 20));
    // 设置默认的初始拥塞窗口大小为10
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    //设置默认延迟ack数量为1
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));
    //设置默认tcp段大小为524Bytes (设置较小的tcp段，以防止IP分片)
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segmentSize));
    //开启选择性重传
    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(isSack));

    int staNum = 3;
    bool verbose = false;
    bool tracing = true;
    double stopNum = 0;
    CommandLine cmd;
    cmd.AddValue("verbose", "tell Application includes http and ftp to log if true", verbose);
    cmd.AddValue("nSta", "Number of stations node", staNum);
    cmd.AddValue("tracing", "Enable pcap and ascii tracing", tracing);
    cmd.AddValue("stopTime", "set the time stop simulate, recommended over 100", stopNum);
    cmd.Parse(argc, argv);
    if (staNum > 10)
    {
        std::cout << "staion number exceeds limits" << std::endl;
        return 1;
    }
    if (verbose)
    {
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("ThreeGppHttpClient", LOG_INFO);
        LogComponentEnable("ThreeGppHttpServer", LOG_INFO);
    }
    if (stopNum != 0)
    {
        stopTime = Seconds(stopNum);
    }
    NodeContainer routerNodes;
    routerNodes.Create(4);
    NodeContainer clientSwitchNode, serverSwitchNode;
    serverSwitchNode.Create(1);
    clientSwitchNode.Create(1);
    NodeContainer serverNodes;
    serverNodes.Create(2);
    NodeContainer apNodes, staNodes;
    apNodes.Create(4);
    staNodes.Create(staNum);

    //主干网络互联节点
    NodeContainer net01(routerNodes.Get(0), routerNodes.Get(1));
    NodeContainer net02(routerNodes.Get(0), routerNodes.Get(2));
    NodeContainer net12(routerNodes.Get(1), routerNodes.Get(2));
    NodeContainer net13(routerNodes.Get(1), routerNodes.Get(3));
    NodeContainer net23(routerNodes.Get(2), routerNodes.Get(3));

    //client网络互联节点
    NodeContainer clientRS(routerNodes.Get(0), clientSwitchNode.Get(0));
    NodeContainer switchAp0(clientSwitchNode.Get(0), apNodes.Get(0));
    NodeContainer switchAp1(clientSwitchNode.Get(0), apNodes.Get(1));
    NodeContainer switchAp2(clientSwitchNode.Get(0), apNodes.Get(2));
    NodeContainer switchAp3(clientSwitchNode.Get(0), apNodes.Get(3));

    //server网络互联节点
    NodeContainer serverRS(routerNodes.Get(3), serverSwitchNode.Get(0));
    NodeContainer switchServerFtp(serverSwitchNode.Get(0), serverNodes.Get(0));
    NodeContainer switchServerWeb(serverSwitchNode.Get(0), serverNodes.Get(1));

    CsmaHelper csmaHelper;
    //设置主干网络信道速率
    csmaHelper.SetChannelAttribute("DataRate", StringValue("1000Mbps"));
    csmaHelper.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer ndc01 = csmaHelper.Install(net01);
    NetDeviceContainer ndc02 = csmaHelper.Install(net02);
    NetDeviceContainer ndc12 = csmaHelper.Install(net12);
    NetDeviceContainer ndc13 = csmaHelper.Install(net13);
    NetDeviceContainer ndc23 = csmaHelper.Install(net23);
    NetDeviceContainer clientRSdc = csmaHelper.Install(clientRS);
    NetDeviceContainer serverRSdc = csmaHelper.Install(serverRS);

    //client网络信道速率
    csmaHelper.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csmaHelper.SetChannelAttribute("Delay", StringValue("10ms"));
    NetDeviceContainer switchApdc0 = csmaHelper.Install(switchAp0);
    NetDeviceContainer switchApdc1 = csmaHelper.Install(switchAp1);
    NetDeviceContainer switchApdc2 = csmaHelper.Install(switchAp2);
    NetDeviceContainer switchApdc3 = csmaHelper.Install(switchAp3);

    //server网络信道速率
    csmaHelper.SetChannelAttribute("DataRate", StringValue("400Mbps"));
    csmaHelper.SetChannelAttribute("Delay", StringValue("5ms"));
    NetDeviceContainer switchServerFtpdv = csmaHelper.Install(switchServerFtp);
    NetDeviceContainer switchServerWebdv = csmaHelper.Install(switchServerWeb);

    //设置wlan 注意需要不同ap点使用不同的channel，不然会有冲突，在两个ap中间的sta会因为conflict无法接收信息
    NetDeviceContainer apDevices, staDevices;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(25));
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    staDevices.Add(wifi.Install(phy, mac, staNodes));

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));
    apDevices.Add(wifi.Install(phy, mac, apNodes));

    //设置交换机先是clientside
    BridgeHelper bridge;
    NetDeviceContainer apBridge_0(apDevices.Get(0), switchApdc0.Get(1));
    NetDeviceContainer apBridge_1(apDevices.Get(1), switchApdc1.Get(1));
    NetDeviceContainer apBridge_2(apDevices.Get(2), switchApdc2.Get(1));
    NetDeviceContainer apBridge_3(apDevices.Get(3), switchApdc3.Get(1));
    NetDeviceContainer clientSwitchdv(clientRSdc.Get(1), switchApdc0.Get(0));
    clientSwitchdv.Add(switchApdc1.Get(0));
    clientSwitchdv.Add(switchApdc2.Get(0));
    clientSwitchdv.Add(switchApdc3.Get(0));
    bridge.Install(apNodes.Get(0), apBridge_0);
    bridge.Install(apNodes.Get(1), apBridge_1);
    bridge.Install(apNodes.Get(2), apBridge_2);
    bridge.Install(apNodes.Get(3), apBridge_3);
    bridge.Install(clientSwitchNode.Get(0), clientSwitchdv);
    //serverside 交换机
    NetDeviceContainer serverSwitchdv(serverRSdc.Get(1), switchServerFtpdv.Get(0));
    serverSwitchdv.Add(switchServerWebdv.Get(0));
    bridge.Install(serverSwitchNode.Get(0), serverSwitchdv);

    //设置各个点的位置
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(50.0),
                                  "MinY", DoubleValue(10.0),
                                  "DeltaX", DoubleValue(10.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(6),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.Install(routerNodes);
    mobility.Install(serverSwitchNode);
    mobility.Install(serverNodes);
    mobility.Install(clientSwitchNode);
    Ptr<MobilityModel> mob0 = serverNodes.Get(1)->GetObject<MobilityModel>();
    mob0->SetPosition(Vector(100, 15, 0));
    Ptr<MobilityModel> mob1 = clientSwitchNode.Get(0)->GetObject<MobilityModel>();
    mob1->SetPosition(Vector(60, 20, 0));

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(35.0),
                                  "MinY", DoubleValue(55.0),
                                  "DeltaX", DoubleValue(50.0),
                                  "DeltaY", DoubleValue(50.0),
                                  "GridWidth", UintegerValue(2),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.Install(apNodes);

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(10, 110, 30, 130)));
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X", StringValue("ns3::UniformRandomVariable[Min=10|Max=110]"),
                                  "Y", StringValue("ns3::UniformRandomVariable[Min=30|Max=130]"));
    mobility.Install(staNodes);

    //设置主干网络Rip路由
    RipHelper ripRouting;
    //去掉不需要的interface
    ripRouting.ExcludeInterface(routerNodes.Get(0), 3);
    ripRouting.ExcludeInterface(routerNodes.Get(3), 3);
    //设置路由2和路由3之间的损耗为10，其他路由之间的默认为1
    ripRouting.SetInterfaceMetric(routerNodes.Get(2), 3, 10);
    ripRouting.SetInterfaceMetric(routerNodes.Get(3), 2, 10);

    Ipv4ListRoutingHelper listRH;
    listRH.Add(ripRouting, 0);

    InternetStackHelper internet;
    internet.SetIpv6StackInstall(false);
    internet.SetRoutingHelper(listRH);
    internet.Install(routerNodes);

    InternetStackHelper intranet;
    intranet.SetIpv6StackInstall(false);
    intranet.Install(staNodes);
    intranet.Install(serverNodes);
    intranet.Install(clientSwitchNode);

    //链路层流量控制
    TrafficControlHelper tch;
    tch.SetRootQueueDisc(qdiscTypeId);
    QueueDiscContainer qd;
    //主要是在交换机和Ap之间，有信道速率的不匹配，需要进行控制
    for (int i = 2; i < 5; i++)
    {
        //tch.Uninstall(clientSwitchNode.Get(0)->GetDevice(i));
        qd.Add(tch.Install(clientSwitchNode.Get(0)->GetDevice(i)));
    }
    tch.SetQueueLimits("ns3::DynamicQueueLimits");

    //分配IP地址
    Ipv4AddressHelper ipv4;
    //主干网络ip地址
    ipv4.SetBase(Ipv4Address("10.0.0.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic01 = ipv4.Assign(ndc01);
    ipv4.SetBase(Ipv4Address("10.0.1.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic02 = ipv4.Assign(ndc02);
    ipv4.SetBase(Ipv4Address("10.0.2.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic12 = ipv4.Assign(ndc12);
    ipv4.SetBase(Ipv4Address("10.0.3.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic13 = ipv4.Assign(ndc13);
    ipv4.SetBase(Ipv4Address("10.0.4.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic23 = ipv4.Assign(ndc23);
    //客户端网络ip分配
    Ipv4InterfaceContainer iicClient;
    NetDeviceContainer channelDevices;
    ipv4.SetBase(Ipv4Address("10.0.5.0"), Ipv4Mask("255.255.255.0"));
    channelDevices.Add(clientRSdc.Get(0));
    for (int j = 0; j < staNum; j++)
    {
        channelDevices.Add(staDevices.Get(j));
    }
    iicClient = ipv4.Assign(channelDevices);

    //服务器网络ip分配
    NetDeviceContainer serverNetDevices(serverRSdc.Get(0), switchServerFtpdv.Get(1));
    serverNetDevices.Add(switchServerWebdv.Get(1));
    ipv4.SetBase(Ipv4Address("10.0.6.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iicServer = ipv4.Assign(serverNetDevices);

    //静态ip路由，servers和clients
    Ptr<Ipv4StaticRouting> staticRoutingServer;
    Ptr<Ipv4StaticRouting> staticRoutingClient;
    for (int i = 0; i < 2; i++)
    {
        staticRoutingServer = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(serverNodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol());
        staticRoutingServer->SetDefaultRoute("10.0.6.1", 1);
    }
    for (int i = 0; i < staNum; i++)
    {
        staticRoutingClient = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(staNodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol());
        staticRoutingClient->SetDefaultRoute("10.0.5.1", 1);
        //staticRoutingClient->AddNetworkRouteTo(Ipv4Address("10.0.6.0"), Ipv4Mask("255.255.255.0"), 1);
    }

    // UdpEchoServerHelper echoServer(21);
    // ApplicationContainer serverApps = echoServer.Install(serverNodes.Get(0));
    // //Ptr<Ipv4> sta1IP = staNodes.Get(1)->GetObject<Ipv4>();
    // //std::cout << "Ip address: " << sta1IP->GetAddress(1, 0);
    // serverApps.Start(Seconds(2.0));
    // serverApps.Stop(stopTime);
    // UdpEchoClientHelper echoClient(iicServer.GetAddress(1), 21);
    // echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    // echoClient.SetAttribute("Interval", TimeValue(Seconds(5.0)));
    // echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // ApplicationContainer clientApps = echoClient.Install(staNodes.Get(2));
    // clientApps.Start(Seconds(10.0));
    // clientApps.Stop(Seconds(7));
    // clientApps.Start(Seconds(10));
    // clientApps.Stop(stopTime);

    //设置web服务器
    ThreeGppHttpServerHelper httpHelper(iicServer.GetAddress(2));
    ApplicationContainer httpServerApps = httpHelper.Install(serverNodes.Get(1));
    httpServerApps.Start(Seconds(5.0));
    httpServerApps.Stop(stopTime);
    //http server trace设置
    Ptr<ThreeGppHttpServer> httpServer = httpServerApps.Get(0)->GetObject<ThreeGppHttpServer>();
    // Example of connecting to the trace sources
    //   httpServer->TraceConnectWithoutContext ("ConnectionEstablished",
    //                                           MakeCallback (&ServerConnectionEstablished));
    //   httpServer->TraceConnectWithoutContext ("MainObject", MakeCallback (&MainObjectGenerated));
    //   httpServer->TraceConnectWithoutContext ("EmbeddedObject", MakeCallback (&EmbeddedObjectGenerated));
    //   httpServer->TraceConnectWithoutContext ("Tx", MakeCallback (&ServerTx));
    // Setup HTTP variables for the server
    PointerValue varPtr;
    httpServer->GetAttribute("Variables", varPtr);
    Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables>();
    httpVariables->SetMainObjectSizeMean(102400);  // 100kB
    httpVariables->SetMainObjectSizeStdDev(40960); // 40kB

    ThreeGppHttpClientHelper httpClientHelper(iicServer.GetAddress(2));
    ApplicationContainer httpClientApps = httpClientHelper.Install(staNodes.Get(0));
    httpClientApps.Start(Seconds(10.0));
    httpClientApps.Stop(stopTime);

    //设置Ftp服务器，使用bulkSend来模拟，只设置一个客户端
    uint16_t port = 21;
    Address ftpClientAddress(InetSocketAddress(iicClient.GetAddress(staNum), port));
    BulkSendHelper ftpHelper("ns3::TcpSocketFactory", ftpClientAddress);
    ftpHelper.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer ftpServer = ftpHelper.Install(serverNodes.Get(0));
    ftpServer.Start(Seconds(30.0));
    ftpServer.Stop(stopTime);

    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", ftpClientAddress);
    ApplicationContainer ftpClientApps = sinkHelper.Install(staNodes.Get(staNum - 1));
    ftpClientApps.Start(Seconds(5.0));
    ftpClientApps.Stop(stopTime);

    RipHelper routingHelper;
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("final_test.routes", std::ios::out);
    routingHelper.PrintRoutingTableAt(Seconds(10.0), routerNodes.Get(0), routingStream);
    routingHelper.PrintRoutingTableAt(Seconds(10.0), staNodes.Get(0), routingStream);

    if (tracing)
    {

        //phy.EnablePcap("final_test_web_client", staDevices.Get(1));
        csmaHelper.EnablePcap("final_test_client_router", clientRSdc.Get(0));
        phy.EnablePcap("final_test_ap_0", apDevices.Get(0));
        phy.EnablePcap("final_test_ap_1", apDevices.Get(1));
        phy.EnablePcap("final_test_ap_2", apDevices.Get(2));
        phy.EnablePcap("final_test_client", staDevices.Get(1));
        phy.EnablePcap("final_test_ftp_client", staDevices.Get(staNum - 1));
        csmaHelper.EnablePcap("final_test_server_router", serverRSdc.Get(0));
        csmaHelper.EnablePcap("final_test_web_server", switchServerWebdv.Get(1));
        csmaHelper.EnableAscii("final_test_web_server", switchServerWebdv.Get(1));
        csmaHelper.EnablePcap("final_test_ftp_server", switchServerFtpdv.Get(1));
        csmaHelper.EnableAscii("final_test_ftp_server", switchServerFtpdv.Get(1));
        // csmaHelper.EnablePcap("final_test_router_0", clientRSdc.Get(0));
        //csmaHelper.EnablePcap("final_test_router_0", ndc01.Get(0));
    }
    AnimationInterface anim("final_test.xml");
    anim.SetMaxPktsPerTraceFile(999999999999);
    Simulator::Stop(stopTime);
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}