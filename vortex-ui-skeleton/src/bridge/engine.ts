import { Vortex } from './vortex';

type CEFAny = any;

export type NodeTypeItem = {
  type?: string;
  category?: string;
  icon?: string;
  // remaining fields from serialized info — as is
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
  // Using unified bridge from vortex.ts
  private V = Vortex;

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
    // In CEF we might get JSON string of entire dict OR object where values are JSON strings
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

  // --- API, 1:1 matching App::_message_handlers_disp ---
  async getNodeTypes(): Promise<Record<string, NodeTypeItem>> {
    const r = await this.V.getNodeTypes();
    const dict = this.normalizeDict<NodeTypeItem>(r);
    // set type field just in case
    for (const [k, v] of Object.entries(dict)) {
      (v as any).type ??= k;
    }
    return dict;
  }

  async createNode(typeName: string): Promise<number> {
    const ptr = await this.V.createNode(typeName);
    return Number(ptr);
  }

  async getNodeProperties(nodePtr: number): Promise<PropSchema> {
    const s = await this.V.getNodeProperties(nodePtr);
    const parsed = this.parseJSON<PropSchema>(s);
    return parsed ?? { properties: [] };
  }

  setNodePropertyByName(nodePtr: number, name: string, jsonValue: any) {
    this.V.setNodeProperty(nodePtr, name, JSON.stringify(jsonValue));
  }

  setNodeProperty(nodePtr: number, index: number, jsonValue: any) {
    this.V.setNodeProperty(nodePtr, index, JSON.stringify(jsonValue));
  }

  async connect(l: number, lo: number, r: number, ri: number): Promise<boolean> {
    const ok = await this.V.connect(l, lo, r, ri);
    return String(ok) === 'true' || ok === true;
  }

  disconnect(l: number, lo: number, r: number, ri: number) {
    this.V.disconnect(l, lo, r, ri);
  }

  removeNode(ptr: number) {
    this.V.removeNode(ptr);
  }

  play() {
    this.V.play();
  }
  stop() {
    this.V.stop();
  }

  // event from C++: App::OnNodeUpdate -> SendUIMessage("node_update", ...)
  onNodeUpdate(cb: (nodePtr: number, propIndex: number, value: any) => void) {
    const handler = (ev: CEFAny) => {
      const { name, args } = ev.detail || {};
      if (name !== 'node_update' || !Array.isArray(args) || args.length < 3) {
        return;
      }

      const nodePtr = Number(args[0]);
      const propIdx = Number(args[1]);
      const raw = args[2];

      try {
        cb(nodePtr, propIdx, JSON.parse(raw));
      } catch {
        cb(nodePtr, propIdx, raw);
      }
    };

    (window as any).addEventListener?.('cef-message', handler);
    return () => (window as any).removeEventListener?.('cef-message', handler);
  }
}

export const engine = new CefEngine();
