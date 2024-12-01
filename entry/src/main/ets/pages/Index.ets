import nativeNapi from 'libentry.so';
import { connection } from '@kit.NetworkKit';

function get_nets() {
  let res: Map<string, connection.ConnectionProperties> = new Map();
  const nets = connection.getAllNetsSync();
  for (const net of nets) {
    let prop = connection.getConnectionPropertiesSync(net);
    res.set(prop.interfaceName, prop);
  }
  return res;
}

// get interface info in `ip a` format
function format_addrs(intf: nativeNapi.Interface[]) {
  intf = intf.sort((a, b) => a.name < b.name ? -1 : (a.name > b.name ? 1 : 0));
  let res: string[] = [];
  let i = 1;
  const nets = get_nets();
  for (let entry of intf) {
    let net = nets.get(entry.name);
    if (net !== undefined) {
      res.push(`${i}: ${entry.name}: <${entry.flags}> mtu ${net.mtu}`);
    } else {
      res.push(`${i}: ${entry.name}: <${entry.flags}>`);
    }
    if (entry.mac !== undefined) {
      res.push(`    link/ether ${entry.mac} brd ff:ff:ff:ff:ff:ff`);
    }
    if (entry.addrs !== undefined) {
      for (let addr of entry.addrs) {
        if (addr.ipv4_addr !== undefined) {
          res.push(`    inet ${addr.ipv4_addr}/${addr.prefix} brd ${addr.ipv4_broadaddr}`);
        } else if (addr.ipv6_addr !== undefined) {
          res.push(`    inet6 ${addr.ipv6_addr}/${addr.prefix}`);
        }
      }
    }
    i += 1;
  }
  return res;
}

// get routing table in `ip r` format + dns info
function get_routes() {
  let res: string[] = [];
  const nets = get_nets();
  res.push(`Routing Table:`);
  for (const net of nets) {
    for (const route of net[1].routes) {
      res.push(`${route.destination.address.address}/${route.destination.prefixLength} via ${route.gateway.address} dev ${route.interface}`);
    }
  }

  for (const net of nets) {
    res.push(`DNS Servers for ${net[0]}:`);
    for (const dns of net[1].dnses) {
      res.push(`${dns.address}`);
    }
  }
  return res;
}

@Entry
@Component
struct Index {
  // https://www.cnblogs.com/wgjava/p/18454731
  controller: TabsController = new TabsController()
  @State current: number = 0

  @Builder
  tabBuilder(index: number, label: string) {
    Column() {
      Text(label)
        .fontSize('12fp')
        .fontColor(this.current === index ? '#909090' : '#000000')
        .margin({ top: 3 })
    }
    .width('100%')
    .onClick(() => {
      this.current = index
      this.controller.changeIndex(this.current)
    })
  }

  @State intf: string[] =
    format_addrs(nativeNapi.get_intf_addrs());
  @State routes: string[] =
    get_routes();

  @Builder
  listBuilder(data: string[]) {
    List() {
      ForEach(data, (item: string) => {
        ListItem() {
          Row() {
            Text(item).fontSize(10).fontFamily('monospace')
          }
          .width('100%')
          .justifyContent(FlexAlign.Start)
        }
      })
    }
  }

  build() {
    Column() {
      Tabs({ barPosition: BarPosition.End, controller: this.controller }) {
        TabContent() {
          this.listBuilder(this.intf)
        }.tabBar(this.tabBuilder(0, 'Interface'))

        TabContent() {
          this.listBuilder(this.routes)
        }.tabBar(this.tabBuilder(1, 'Routing & DNS'))
      }
      .width('100%')
      .backgroundColor('#f2f2f2')
      .barMode(BarMode.Fixed)
      .scrollable(true)
      .onChange(((index: number) => {
        this.current = index
      }))
    }
    .width('100%')
    .backgroundColor('#ffffff')
  }
}
