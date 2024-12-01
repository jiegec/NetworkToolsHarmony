declare interface Interface {
    ipv4_addr: string | undefined;
    ipv4_broadaddr: string | undefined;
    ipv6_addr: string | undefined;
    ipv6_broadaddr: string | undefined;
    prefix: number;
    mac: string | undefined;
}
declare interface Interface {
    name: string;
    flags: string;
    addrs: Address[] | undefined;
}
export const getifaddrs: () => Interface[];