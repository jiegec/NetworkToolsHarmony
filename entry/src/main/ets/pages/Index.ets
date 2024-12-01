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

  build() {
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
  }
}
