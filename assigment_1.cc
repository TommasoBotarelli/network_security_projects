#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/radvd-interface.h"
#include "ns3/radvd-prefix.h"
#include "ns3/radvd.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("assigment_1");

int
main(int argc, char** argv)
{
    // Creazione dei nodi
    // h1, h2, h3 saranno gli hosts
    // r sarà il router
    Ptr<Node> h1 = CreateObject<Node>();
    Ptr<Node> r = CreateObject<Node>();
    Ptr<Node> h2 = CreateObject<Node>();
    Ptr<Node> h3 = CreateObject<Node>();

    // Creazione dei container per i nodi
    NodeContainer net1(h1, r, h3);
    NodeContainer net2(r, h2);
    NodeContainer all(h1, r, h2, h3);

    // Installazione dello stack ipv6
    InternetStackHelper internetv6;
    internetv6.Install(all);

    // Creazione del canale
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer d1 = csma.Install(net1); // h1 - R - h3
    NetDeviceContainer d2 = csma.Install(net2); // R - h2

    // Creazione rete ed assegnamento dell'ip address
    Ipv6AddressHelper ipv6;

    // Prima subnet h1 - R - h3
    ipv6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    NetDeviceContainer netDevice1;
    netDevice1.Add(d1.Get(0));
    netDevice1.Add(d1.Get(2));                                           
    Ipv6InterfaceContainer iic1 = ipv6.AssignWithoutAddress(netDevice1); 

    NetDeviceContainer netDevice2;
    netDevice2.Add(d1.Get(1));
    Ipv6InterfaceContainer iicr1 = ipv6.Assign(netDevice2);
    iicr1.SetForwarding(0, true);
    iic1.Add(iicr1);

    // Seconda subnet R - h2
    ipv6.SetBase(Ipv6Address("2001:2::"), Ipv6Prefix(64));
    NetDeviceContainer netDevice3;
    netDevice3.Add(d2.Get(0));                              
    Ipv6InterfaceContainer iicr2 = ipv6.Assign(netDevice3);
    iicr2.SetForwarding(0, true);

    NetDeviceContainer netDevice4;
    netDevice4.Add(d2.Get(1)); 
    Ipv6InterfaceContainer iic2 = ipv6.AssignWithoutAddress(netDevice4);
    iic2.Add(iicr2);

    // Configurazione per il radvd
    RadvdHelper radvdHelper;

    // Aggiunta del nwtwork prefix per le due sottoreti
    radvdHelper.AddAnnouncedPrefix(iic1.GetInterfaceIndex(1), Ipv6Address("2001:1::0"), 64);
    radvdHelper.AddAnnouncedPrefix(iic2.GetInterfaceIndex(1), Ipv6Address("2001:2::0"), 64);

    ApplicationContainer radvdApps = radvdHelper.Install(r);
    radvdApps.Start(Seconds(1.0));
    radvdApps.Stop(Seconds(10.0));

    // PING
    uint32_t packetSize = 1024;
    uint32_t maxPacketCount = 3;

    PingHelper ping1(Ipv6Address("2001:2::200:ff:fe00:5"));  // Ping di h2
    PingHelper ping2(Ipv6Address("2001:1::200:ff:fe00:1"));  // Ping di h1

    ping1.SetAttribute("Count", UintegerValue(maxPacketCount));
    ping1.SetAttribute("Size", UintegerValue(packetSize));
    // PING di h1 verso h2
    ApplicationContainer appPing0_1 = ping1.Install(net1.Get(0));

    ping2.SetAttribute("Count", UintegerValue(maxPacketCount));
    ping2.SetAttribute("Size", UintegerValue(packetSize));
    // PING di h3 verso h1 
    ApplicationContainer appPing0_2 = ping2.Install(net1.Get(2));

    appPing0_1.Start(Seconds(2.0));
    appPing0_2.Start(Seconds(8.0));

    appPing0_1.Stop(Seconds(7.0));
    appPing0_2.Stop(Seconds(15.0));

    csma.EnablePcapAll(std::string("pcap"), false);

    NS_LOG_INFO("Esecuzione della Simulazione.");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Completato.");

    return 0;
}