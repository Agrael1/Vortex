declare global {
  function vortexCall(method: string, ...args: any[]): any;
  function vortexCallAsync(method: string, ...args: any[]): Promise<any>;
}

export const Vortex = new Proxy({}, {
  get(_t, prop: string) {
    const name = String(prop);
    return (...args: any[]) =>
      name.endsWith('Async') ? vortexCallAsync(name, ...args) : vortexCall(name, ...args);
  }
}) as {
  GetNodeTypesAsync(): Promise<Record<string, string>>;
  CreateNodeAsync(type: string): Promise<number>;
  GetNodePropertiesAsync(nodePtr: number): Promise<string>;
  SetNodeProperty(nodePtr: number, index: number, value: string): void;
  SetNodePropertyByName(nodePtr: number, name: string, value: string): void;
  ConnectNodesAsync(l: number, lo: number, r: number, ri: number): Promise<boolean>;
  DisconnectNodes(l: number, lo: number, r: number, ri: number): void;
  RemoveNode(nodePtr: number): void;
  SetNodeInfo(nodePtr: number, info: string): void;
  CreateAnimationAsync(nodePtr: number): Promise<number>;
  AddPropertyTrackAsync(anim: number, prop: string, keys?: string): Promise<number>;
  AddKeyframe(track: number, json: string): void;
  Play(): void;
  Stop(): void;
};
