// src/bridge/engine.ts
type CEFAny = any;

function ensureVortexProxy() {
  if ((window as any).Vortex) return (window as any).Vortex;

  (window as any).vortexCall = (method: string, ...args: any[]) =>
    (window as any).cefQuery?.({ request: JSON.stringify({ method, args }) });

  (window as any).vortexCallAsync = (method: string, ...args: any[]) =>
    new Promise((resolve, reject) => {
      (window as any).cefQuery?.({
        request: JSON.stringify({ async: true, method, args }),
        onSuccess: (r: string) => resolve(r),
        onFailure: (_code: number, msg: string) => reject(new Error(msg)),
      });
    });

  (window as any).Vortex = new Proxy(
    {},
    {
      get(_t, k) {
        const name = String(k);
        if (name.endsWith('Async')) {
          return (...args: any[]) => (window as any).vortexCallAsync(name, ...args);
        }
        return (...args: any[]) => (window as any).vortexCall(name, ...args);
      },
    }
  );

  return (window as any).Vortex;
}

export type NodeTypeItem = {
  type?: string;
  category?: string;
  icon?: string;
  // остальные поля из сериализованного info — как есть
  [k: string]: any;
};

export type PropSpec = {
  name: string;
  label?: string;
  type: 'int' | 'float' | 'bool' | 'string' | 'enum' | 'color' | 'vec2' | 'vec3' | 'vec4';
  index: number;
  default?: any;
  min?: number;
  max?: number;
  step?: number;
  enum?: { label: string; value: any }[] | null;
  value?: any;
};
export type PropSchema = { properties: PropSpec[] };

class CefEngine {
  private V = ensureVortexProxy();

  // --- helpers ---
  private parseJSON<T = any>(v: any): T | undefined {
    if (typeof v === 'string') {
      try {
        return JSON.parse(v) as T;
      } catch {
        return undefined;
      }
    }
    return v as T;
  }

  private normalizeDict<T = any>(dictLike: any): Record<string, T> {
    // В CEF может прийти строка JSON всего словаря ИЛИ объект, где значения — строки JSON
    const out: Record<string, T> = {};
    const raw = this.parseJSON<Record<string, any>>(dictLike) ?? dictLike;

    if (raw && typeof raw === 'object') {
      for (const [k, v] of Object.entries(raw)) {
        const parsed = this.parseJSON<T>(v) ?? (v as T);
        out[k] = parsed!;
      }
    }
    return out;
  }

  // --- API, 1:1 с App::_message_handlers_disp ---
  async getNodeTypes(): Promise<Record<string, NodeTypeItem>> {
    const r = await this.V.GetNodeTypesAsync();
    const dict = this.normalizeDict<NodeTypeItem>(r);
    // проставим поле type на всякий случай
    for (const [k, v] of Object.entries(dict)) {
      (v as any).type ??= k;
    }
    return dict;
  }

  async createNode(typeName: string): Promise<number> {
    const ptr = await this.V.CreateNodeAsync(typeName);
    return Number(ptr);
  }

  async getNodeProperties(nodePtr: number): Promise<PropSchema> {
    const s = await this.V.GetNodePropertiesAsync(nodePtr);
    const parsed = this.parseJSON<PropSchema>(s);
    return parsed ?? { properties: [] };
  }

  setNodePropertyByName(nodePtr: number, name: string, jsonValue: any) {
    this.V.SetNodePropertyByName(nodePtr, name, JSON.stringify(jsonValue));
  }

  setNodeProperty(nodePtr: number, index: number, jsonValue: any) {
    this.V.SetNodeProperty(nodePtr, index, JSON.stringify(jsonValue));
  }

  async connect(l: number, lo: number, r: number, ri: number): Promise<boolean> {
    const ok = await this.V.ConnectNodesAsync(l, lo, r, ri);
    return String(ok) === 'true' || ok === true;
  }

  disconnect(l: number, lo: number, r: number, ri: number) {
    this.V.DisconnectNodes(l, lo, r, ri);
  }

  removeNode(ptr: number) {
    this.V.RemoveNode(ptr);
  }

  play() {
    this.V.Play();
  }
  stop() {
    this.V.Stop();
  }

  // событие от C++: App::OnNodeUpdate -> SendUIMessage("node_update", ...)
  onNodeUpdate(cb: (nodePtr: number, propIndex: number, value: any) => void) {
    (window as any).addEventListener?.('cef-message', (ev: CEFAny) => {
      const { name, args } = ev.detail || {};
      if (name === 'node_update' && Array.isArray(args) && args.length >= 3) {
        const nodePtr = Number(args[0]);
        const propIdx = Number(args[1]);
        const raw = args[2];
        try {
          cb(nodePtr, propIdx, JSON.parse(raw));
        } catch {
          cb(nodePtr, propIdx, raw);
        }
      }
    });
  }
}

export const engine = new CefEngine();
