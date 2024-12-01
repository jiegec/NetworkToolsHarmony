import { hilog } from '@kit.PerformanceAnalysisKit';
import testNapi from 'libentry.so'

@Entry
@Component
struct Index {
  @State intf: object[] =
    testNapi.getifaddrs().sort((a, b) => a["name"] < b["name"] ? -1 : (a["name"] > b["name"] ? 1 : 0))

  build() {
    List() {
      ForEach(this.intf, (item: object) => {
        ListItem() {
          Row() {
            Text(JSON.stringify(item)).fontSize(15)
          }
          .width('100%')
          .justifyContent(FlexAlign.Start)
        }
      })
    }
  }
}
