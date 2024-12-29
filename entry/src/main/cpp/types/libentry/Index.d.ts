declare interface Address {
    ipv4_addr: string | undefined;
    ipv4_broadaddr: string | undefined;
    ipv6_addr: string | undefined;
    ipv6_broadaddr: string | undefined;
    prefix: number;
}
declare interface Interface {
    name: string;
    flags: string;
    mac: string | undefined;
    addrs: Address[] | undefined;
}
export const get_intf_addrs: () => Interface[];
