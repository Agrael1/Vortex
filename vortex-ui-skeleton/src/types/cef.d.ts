export {}

declare global {
  interface Window {
    Vortex?: Record<string, (...args: any[]) => any>
    cefQuery?: (args: {
      request: string
      onSuccess?: (response: string) => void
      onFailure?: (code: number, msg: string) => void
    }) => void

    __GraphPanelAddNode?: (typeName: string, ptr: number) => void
  }
}
