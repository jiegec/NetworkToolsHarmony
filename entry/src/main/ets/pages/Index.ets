import { hilog } from '@kit.PerformanceAnalysisKit';
import testNapi from 'libentry.so'

function format_addrs(intf: testNapi.Interface[]) {
  intf = intf.sort((a, b) => a.name < b.name ? -1 : (a.name > b.name ? 1 : 0));
  let res: string[] = [];
  let i = 1;
  for (let entry of intf) {
    res.push(`${i}: ${entry.name}: <${entry.flags}>`);
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

@Entry
@Component
struct Index {
  @State intf: string[] =
    format_addrs(testNapi.get_intf_addrs());
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

  build() {
    Column() {
      Tabs({ barPosition: BarPosition.End, controller: this.controller }) {
        TabContent() {
          List() {
            ForEach(this.intf, (item: string) => {
              ListItem() {
                Row() {
                  Text(item).fontSize(10).fontFamily('monospace')
                }
                .width('100%')
                .justifyContent(FlexAlign.Start)
              }
            })
          }
        }.tabBar(this.tabBuilder(0, 'Interface'))

        TabContent() {
          Text('TODO').backgroundColor('#f2f2f2')
        }.tabBar(this.tabBuilder(1, 'Ping'))
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
