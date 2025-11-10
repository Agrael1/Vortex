export {}

declare global {
  interface Window {
    Vortex?: Record<string, (...args: any[]) => any>
    VortexBridge?: any
    vortex?: any
    vortexCall?: (name: string, ...args: any[]) => boolean
    vortexCallAsync?: (name: string, ...args: any[]) => Promise<any>
    external?: { invoke?: (req: string) => string }
    cefQuery?: (args: {
      request: string
      onSuccess?: (response: string) => void
      onFailure?: (code: number, msg: string) => void
    }) => void

    __GraphPanelAddNode?: (typeName: string, x?: number, y?: number) => Promise<number>
  }
}
