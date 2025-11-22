export {};

type VortexBridgeFn = (method: string, ...args: unknown[]) => unknown;
type VortexCallFn = (name: string, ...args: unknown[]) => unknown;
type VortexCallAsyncFn = (name: string, ...args: unknown[]) => Promise<unknown>;

declare global {
  interface Window {
    Vortex?: Record<string, (...args: unknown[]) => unknown>;
    VortexBridge?: VortexBridgeFn;
    vortex?: unknown;
    vortexCall?: VortexCallFn;
    vortexCallAsync?: VortexCallAsyncFn;
    external?: { invoke?: (req: string) => string };
    cefQuery?: (args: { request: string; onSuccess?: (response: string) => void; onFailure?: (code: number, msg: string) => void }) => void;

    __GraphPanelAddNode?: (typeName: string, x?: number, y?: number) => Promise<number>;
  }
}
