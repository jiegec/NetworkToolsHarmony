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
      for (let addr of entry.addrs as nativeNapi.Address[]) {
        if (addr.ipv4_addr !== undefined) {
          res.push(`    inet ${addr.ipv4_addr}/${addr.prefix} brd ${addr.ipv4_broadaddr}`);
        } else if (addr.ipv6_addr !== undefined) {
          res.push(`    inet6 ${addr.ipv6_addr}/${addr.prefix}`);
        }
      }
    }
    i += 1;
  }
  return res.join('\n');
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
  return res.join('\n');
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

  @State intf: string = '';
  @State routes: string = '';

  update() {
    this.intf = format_addrs(nativeNapi.get_intf_addrs());
    this.routes = get_routes();
  }

  private conn: connection.NetConnection | null = null
  onPageShow() {
    this.update();

    // monitor network change
    const conn: connection.NetConnection = connection.createNetConnection();
    this.conn = conn;
    conn.register((error) => {
      conn.on('netAvailable', () => { this.update(); });
      conn.on('netBlockStatusChange', () => { this.update(); });
      conn.on('netCapabilitiesChange', () => { this.update(); });
      conn.on('netConnectionPropertiesChange', () => { this.update(); });
      conn.on('netLost', () => { this.update(); });
      conn.on('netUnavailable', () => { this.update(); });
    });
  }

  onPageHide() {
    if (this.conn !== null) {
      this.conn.unregister((error) => {});
    }
  }

  build() {
    Column() {
      Tabs({ barPosition: BarPosition.End, controller: this.controller }) {
        TabContent() {
          Scroll() {
            Text(this.intf).fontSize(10).fontFamily('monospace').width('100%')
          }
        }.tabBar(this.tabBuilder(0, 'Interface'))

        TabContent() {
          Text(this.routes).fontSize(10).fontFamily('monospace').width('100%')
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
