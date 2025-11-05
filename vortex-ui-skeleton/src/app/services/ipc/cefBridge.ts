export type ProjectDTO = {
  version: string
  name: string
  path?: string
  settings: { width: number; height: number; fps: number; colorSpace: string }
  assets: { id: string; type: string; path: string }[]
  graph: { nodes: NodeDTO[]; edges: EdgeDTO[] }
}

export type NodeDTO = {
  id: string
  type: string
  params: Record<string, unknown>
  pos: [number, number]
}

export type EdgeDTO = { id: string; from: string; to: string }

type Recent = { name: string; path: string; last: string }

// Simple in-memory mock for now.
class EngineMock {
  private recents: Recent[] = [
    { name: 'Demo Show', path: 'C:/Projects/Vortex/Demo', last: new Date().toISOString() },
    { name: 'NDI Bridge', path: 'D:/Work/Vortex/Bridge', last: new Date(Date.now()-86400000).toISOString() },
  ]

  async listRecent(): Promise<Recent[]> {
    return this.recents
  }

  async openProject(path: string): Promise<ProjectDTO> {
    return {
      version: '1.0',
      name: 'Opened Project',
      path,
      settings: { width: 1920, height: 1080, fps: 60, colorSpace: 'sRGB' },
      assets: [],
      graph: { nodes: [], edges: [] }
    }
  }

  async saveProject(_dto: ProjectDTO): Promise<void> {
    return
  }

  async runGraph(_dto: ProjectDTO): Promise<{ ok: boolean; diagnostics?: string[] }> {
    return { ok: true }
  }

  async queryNodeLibrary(): Promise<{ type: string; category: string; icon: string; defaults: any }[]> {
    return [
      { type: 'StreamInput', category: 'Inputs', icon: 'Play', defaults: {} },
      { type: 'Blend', category: 'Filters', icon: 'Blend', defaults: { opacity: 0.7 } },
      { type: 'WindowOutput', category: 'Output', icon: 'Monitor', defaults: {} },
    ]
  }

  on(_event: string, _cb: (payload: any)=>void) {
    // real impl will bind CEF events
    return () => {}
  }
}

export const engine = new EngineMock()