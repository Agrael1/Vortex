export type ProjectDTO = {
  version: string;
  name: string;
  path?: string;
  template?: string | null;
  settings: { width: number; height: number; fps: number; colorSpace: string };
  assets: { id: string; type: string; path: string }[];
  graph: { nodes: NodeDTO[]; edges: EdgeDTO[] };
};

export type NodeDTO = {
  id: string;
  type: string;
  params: Record<string, unknown>;
  pos: [number, number];
};

export type EdgeDTO = { id: string; from: string; to: string };

export type Recent = {
  name: string;
  path: string;
  last: string;
  pinned?: boolean;
  template?: string | null;
  width?: number;
  height?: number;
  fps?: number;
  colorSpace?: string;
  preview?: string | null;
  error?: string | null;
};

export type LastProject = {
  name: string;
  path: string;
  lastOpened: string;
  template?: string | null;
};

export type EngineLogEntry = {
  level: 'info' | 'warn' | 'error';
  message: string;
  time: number;
  scope?: string | null;
};

export type CreateProjectPayload = {
  name: string;
  location: string;
  width: number;
  height: number;
  fps: number;
  colorSpace?: string;
  template?: string;
};

type EngineEventHandler<T = unknown> = (payload: T) => void;

type NativeLike = {
  listRecent?: () => Promise<Recent[]>;
  openProject?: (path: string) => Promise<ProjectDTO>;
  createProject?: (payload: CreateProjectPayload) => Promise<ProjectDTO>;
  ListRecentProjectsAsync?: () => Promise<Recent[]>;
  OpenProjectAsync?: (path: string) => Promise<ProjectDTO>;
  CreateProjectAsync?: (payload: CreateProjectPayload) => Promise<ProjectDTO>;
  ShowOpenProjectDialogAsync?: () => Promise<string | null | undefined>;
  ShowSelectFolderDialogAsync?: () => Promise<string | null | undefined>;
  saveProject?: () => Promise<void> | void;
  SaveProjectAsync?: () => Promise<void>;
  on?: (event: string, handler: EngineEventHandler) => () => void;
};

const RECENTS_STORAGE_KEY = 'vortex.hub.recents';
const RECENTS_EVENT = 'vortex.recents';
const LAST_PROJECT_STORAGE_KEY = 'vortex.session.lastProject';
const LAST_PROJECT_EVENT = 'vortex.lastProject';
const LOG_EVENT = 'vortex.log';

const getLocalStorage = (): Storage | null => {
  if (typeof window === 'undefined') return null;
  try {
    return window.localStorage;
  } catch {
    return null;
  }
};

const deriveNameFromPath = (path: string) => {
  if (!path) return 'Untitled';
  const normalized = path.trim().replace(/\\/g, '/');
  const parts = normalized.split('/');
  const last = parts[parts.length - 1] || normalized;
  return last.replace(/\.[^.]+$/, '') || last || 'Untitled';
};

const normalizeRecents = (items: unknown): Recent[] => {
  if (!Array.isArray(items)) return [];
  return items
    .map((item) => {
      if (!item || typeof item !== 'object') return null;
      const rawPath = (item as any).path;
      if (typeof rawPath !== 'string' || !rawPath.length) return null;
      const rawName = (item as any).name;
      const rawLast = (item as any).last;
      const name = typeof rawName === 'string' && rawName.trim().length ? rawName.trim() : deriveNameFromPath(rawPath);
      const last = typeof rawLast === 'string' && rawLast.trim().length ? rawLast : new Date().toISOString();
      const template = typeof (item as any).template === 'string' ? (item as any).template : null;
      const width = Number.isFinite((item as any).width) ? Number((item as any).width) : undefined;
      const height = Number.isFinite((item as any).height) ? Number((item as any).height) : undefined;
      const fps = Number.isFinite((item as any).fps) ? Number((item as any).fps) : undefined;
      const colorSpace = typeof (item as any).colorSpace === 'string' ? (item as any).colorSpace : undefined;
      const preview = typeof (item as any).preview === 'string' ? (item as any).preview : null;
      const error = typeof (item as any).error === 'string' ? (item as any).error : null;
      const pinned = (item as any).pinned === true;
      const entry: Recent = {
        name,
        path: rawPath,
        last,
        template: template ?? undefined,
        width,
        height,
        fps,
        colorSpace,
        preview,
        error,
      };

      if (pinned) {
        entry.pinned = true;
      }

      if (template === null) {
        entry.template = null;
      }

      return entry;
    })
    .filter((item): item is Recent => Boolean(item));
};

const readStoredRecents = (): Recent[] => {
  const storage = getLocalStorage();
  if (!storage) return [];
  const raw = storage.getItem(RECENTS_STORAGE_KEY);
  if (!raw) return [];
  try {
    return normalizeRecents(JSON.parse(raw));
  } catch {
    return [];
  }
};

const writeStoredRecents = (recents: Recent[]) => {
  const storage = getLocalStorage();
  if (!storage) return;
  try {
    storage.setItem(RECENTS_STORAGE_KEY, JSON.stringify(recents));
  } catch {
    /* ignore quota issues */
  }
};

const emitRecentsUpdate = (recents: Recent[]) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(RECENTS_EVENT, { detail: recents }));
};

const readLastProject = (): LastProject | null => {
  const storage = getLocalStorage();
  if (!storage) return null;
  const raw = storage.getItem(LAST_PROJECT_STORAGE_KEY);
  if (!raw) return null;

  try {
    const parsed = JSON.parse(raw);
    if (!parsed || typeof parsed !== 'object') return null;
    const name = typeof (parsed as any).name === 'string' ? (parsed as any).name : null;
    const path = typeof (parsed as any).path === 'string' ? (parsed as any).path : null;
    const lastOpened = typeof (parsed as any).lastOpened === 'string' ? (parsed as any).lastOpened : null;
    if (!name || !path || !lastOpened) return null;
    const template = typeof (parsed as any).template === 'string' ? (parsed as any).template : null;
    return { name, path, lastOpened, template };
  } catch {
    return null;
  }
};

const writeLastProject = (entry: Recent | null) => {
  const storage = getLocalStorage();
  if (!storage) return;

  if (!entry) {
    storage.removeItem(LAST_PROJECT_STORAGE_KEY);
    emitLastProjectUpdate(null);
    return;
  }

  const payload: LastProject = {
    name: entry.name,
    path: entry.path,
    lastOpened: entry.last ?? new Date().toISOString(),
    template: entry.template ?? null,
  };

  try {
    storage.setItem(LAST_PROJECT_STORAGE_KEY, JSON.stringify(payload));
  } catch {
    /* ignore quota issues */
  }

  emitLastProjectUpdate(payload);
};

const emitLastProjectUpdate = (project: LastProject | null) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(LAST_PROJECT_EVENT, { detail: project }));
};

const emitLogUpdate = (entry: EngineLogEntry) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(LOG_EVENT, { detail: entry }));
};

class EngineBridge {
  private native: NativeLike | null;
  private static readonly NATIVE_TIMEOUT = 4000;

  constructor() {
    this.native = this.resolveNative();
  }

  private resolveNative(): NativeLike | null {
    if (typeof window === 'undefined') return null;
    const win = window as unknown as Record<string, unknown>;

    const candidates = [win.VortexHub, win.vortex, win.Vortex, win.engine];

    for (const candidate of candidates) {
      if (candidate && typeof candidate === 'object') {
        return candidate as NativeLike;
      }
    }

    const hasVortexCall = typeof (window as any).vortexCallAsync === 'function';
    if (hasVortexCall) {
      const call = (method: string, ...args: unknown[]) => (window as any).vortexCallAsync(method, ...args);
      return {
        listRecent: () => call('ListRecentProjectsAsync'),
        openProject: (path: string) => call('OpenProjectAsync', path),
        createProject: (payload: CreateProjectPayload) => call('CreateProjectAsync', payload),
        ShowOpenProjectDialogAsync: () => call('ShowOpenProjectDialogAsync'),
        ShowSelectFolderDialogAsync: () => call('ShowSelectFolderDialogAsync'),
        saveProject: () => call('SaveProjectAsync'),
        SaveProjectAsync: () => call('SaveProjectAsync'),
      };
    }

    return null;
  }

  private touchRecents(entry: Partial<Recent> & { path: string }) {
    const now = new Date().toISOString();
    const stored = readStoredRecents();
    const existing = stored.find((item) => item.path === entry.path);

    const nextEntry: Recent = {
      name: entry.name?.trim()?.length ? entry.name.trim() : existing?.name || deriveNameFromPath(entry.path),
      path: entry.path,
      last: entry.last ?? existing?.last ?? now,
      pinned: entry.pinned ?? existing?.pinned,
      template: entry.template ?? existing?.template ?? null,
      width: entry.width ?? existing?.width,
      height: entry.height ?? existing?.height,
      fps: entry.fps ?? existing?.fps,
      colorSpace: entry.colorSpace ?? existing?.colorSpace,
      preview: entry.preview ?? existing?.preview ?? null,
      error: entry.error ?? existing?.error ?? null,
    };

    const merged = [nextEntry, ...stored.filter((item) => item.path !== entry.path)];
    const limited = merged.slice(0, 30);
    writeLastProject(nextEntry);
    writeStoredRecents(limited);
    emitRecentsUpdate(limited);
  }

  private ensureProjectStub(path: string, payload?: Partial<CreateProjectPayload>): ProjectDTO {
    const width = payload?.width ?? 1920;
    const height = payload?.height ?? 1080;
    const fps = payload?.fps ?? 60;
    const colorSpace = payload?.colorSpace ?? 'Rec.709';
    const name = payload?.name?.trim().length ? payload.name.trim() : deriveNameFromPath(path);

    return {
      version: '1.0',
      name,
      path,
      template: payload?.template ?? null,
      settings: { width, height, fps, colorSpace },
      assets: [],
      graph: { nodes: [], edges: [] },
    };
  }

  async listRecent(): Promise<Recent[]> {
    const stored = readStoredRecents();

    if (this.native?.listRecent) {
      const received = normalizeRecents(await this.native.listRecent());
      const merged = this.mergeWithStored(received, stored);
      writeStoredRecents(merged.slice(0, 30));
      return merged;
    }

    if (this.native?.ListRecentProjectsAsync) {
      const received = normalizeRecents(await this.native.ListRecentProjectsAsync());
      const merged = this.mergeWithStored(received, stored);
      writeStoredRecents(merged.slice(0, 30));
      return merged;
    }

    return stored;
  }

  async openProject(path: string): Promise<ProjectDTO> {
    if (!path || !path.trim()) {
      throw new Error('Project path must be provided');
    }

    const normalizedPath = path.trim();

    const directProject = await this.tryNativeCall('openProject', () => this.native?.openProject?.(normalizedPath));
    if (directProject?.path) {
      this.touchRecents({
        name: directProject.name,
        path: directProject.path,
        last: new Date().toISOString(),
        template: directProject.template ?? null,
        width: directProject.settings?.width,
        height: directProject.settings?.height,
        fps: directProject.settings?.fps,
        colorSpace: directProject.settings?.colorSpace,
      });
      return directProject;
    }

    const asyncProject = await this.tryNativeCall('OpenProjectAsync', () => this.native?.OpenProjectAsync?.(normalizedPath));
    if (asyncProject?.path) {
      this.touchRecents({
        name: asyncProject.name,
        path: asyncProject.path,
        last: new Date().toISOString(),
        template: asyncProject.template ?? null,
        width: asyncProject.settings?.width,
        height: asyncProject.settings?.height,
        fps: asyncProject.settings?.fps,
        colorSpace: asyncProject.settings?.colorSpace,
      });
      return asyncProject;
    }

    const stub = this.ensureProjectStub(normalizedPath);
    this.touchRecents({
      name: stub.name,
      path: normalizedPath,
      last: new Date().toISOString(),
      template: stub.template ?? null,
      width: stub.settings.width,
      height: stub.settings.height,
      fps: stub.settings.fps,
      colorSpace: stub.settings.colorSpace,
    });
    return stub;
  }

  async createProject(payload: CreateProjectPayload): Promise<ProjectDTO> {
    const location = payload.location?.trim();
    if (!location) {
      throw new Error('Please specify a target location for the project');
    }

    const direct = await this.tryNativeCall('createProject', () => this.native?.createProject?.(payload));
    if (direct?.path) {
      this.touchRecents({
        name: direct.name,
        path: direct.path,
        last: new Date().toISOString(),
        template: direct.template ?? payload.template ?? null,
        width: direct.settings?.width,
        height: direct.settings?.height,
        fps: direct.settings?.fps,
        colorSpace: direct.settings?.colorSpace,
      });
      return direct;
    }

    const asyncResult = await this.tryNativeCall('CreateProjectAsync', () => this.native?.CreateProjectAsync?.(payload));
    if (asyncResult?.path) {
      this.touchRecents({
        name: asyncResult.name,
        path: asyncResult.path,
        last: new Date().toISOString(),
        template: asyncResult.template ?? payload.template ?? null,
        width: asyncResult.settings?.width,
        height: asyncResult.settings?.height,
        fps: asyncResult.settings?.fps,
        colorSpace: asyncResult.settings?.colorSpace,
      });
      return asyncResult;
    }

    const stub = this.ensureProjectStub(location, payload);
    this.touchRecents({
      name: stub.name,
      path: location,
      last: new Date().toISOString(),
      template: stub.template ?? payload.template ?? null,
      width: stub.settings.width,
      height: stub.settings.height,
      fps: stub.settings.fps,
      colorSpace: stub.settings.colorSpace,
    });
    return stub;
  }

  async browseForProject(): Promise<string | null> {
    if (this.native?.ShowOpenProjectDialogAsync) {
      const selected = await this.native.ShowOpenProjectDialogAsync();
      if (typeof selected === 'string') {
        const trimmed = selected.trim();
        return trimmed.length ? trimmed : null;
      }
      return null;
    }

    if (typeof window !== 'undefined') {
      const fallback = window.prompt('Enter absolute path to a Vortex project (*.vortex)');
      if (typeof fallback === 'string') {
        const trimmed = fallback.trim();
        return trimmed.length ? trimmed : null;
      }
    }

    return null;
  }

  async browseForFolder(): Promise<string | null> {
    if (this.native?.ShowSelectFolderDialogAsync) {
      const selected = await this.native.ShowSelectFolderDialogAsync();
      if (typeof selected === 'string') {
        const trimmed = selected.trim();
        return trimmed.length ? trimmed : null;
      }
      return null;
    }

    if (typeof window !== 'undefined') {
      const fallback = window.prompt('Enter destination folder path');
      if (typeof fallback === 'string') {
        const trimmed = fallback.trim();
        return trimmed.length ? trimmed : null;
      }
    }

    return null;
  }

  async saveProject(): Promise<'native' | 'fallback'> {
    const attempt = async (label: 'saveProject' | 'SaveProjectAsync'): Promise<boolean> => {
      if (!this.native) return false;

      const method = label === 'saveProject' ? this.native.saveProject : this.native.SaveProjectAsync;
      if (typeof method !== 'function') return false;

      try {
        await this.withTimeout(Promise.resolve(method.call(this.native)), EngineBridge.NATIVE_TIMEOUT, label);
        return true;
      } catch (error) {
        console.warn(`[EngineBridge] ${label} call failed`, error);
        return false;
      }
    };

    if (await attempt('saveProject')) {
      return 'native';
    }

    if (await attempt('SaveProjectAsync')) {
      return 'native';
    }

    const storage = getLocalStorage();
    if (storage) {
      try {
        storage.setItem('vortex.editor.lastSave', new Date().toISOString());
        console.warn('[EngineBridge] Using fallback save stub – native save not available');
        return 'fallback';
      } catch (error) {
        console.warn('[EngineBridge] Fallback save stub failed', error);
      }
    }

    throw new Error('Save project is not available in the current environment');
  }

  on<T = unknown>(event: string, handler: EngineEventHandler<T>) {
    if (this.native?.on) {
      const off = this.native.on(event, handler as EngineEventHandler);
      if (off) return off;
    }

    if (event === 'recents:updated' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(RECENTS_EVENT, listener as EventListener);
      return () => window.removeEventListener(RECENTS_EVENT, listener as EventListener);
    }

    if (event === 'lastProject:updated' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(LAST_PROJECT_EVENT, listener as EventListener);
      return () => window.removeEventListener(LAST_PROJECT_EVENT, listener as EventListener);
    }

    if (event === 'log' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(LOG_EVENT, listener as EventListener);
      return () => window.removeEventListener(LOG_EVENT, listener as EventListener);
    }

    return () => undefined;
  }

  getLastProject(): LastProject | null {
    return readLastProject();
  }

  clearLastProject() {
    writeLastProject(null);
  }

  emitLog(entry: EngineLogEntry) {
    const payload: EngineLogEntry = {
      level: entry?.level ?? 'info',
      message: typeof entry?.message === 'string' ? entry.message : String(entry?.message ?? ''),
      time: Number.isFinite(entry?.time) ? Number(entry.time) : Date.now(),
      scope: typeof entry?.scope === 'string' ? entry.scope : null,
    };

    emitLogUpdate(payload);
  }

  private mergeWithStored(received: Recent[], stored: Recent[]): Recent[] {
    const merged = received.map((item) => {
      const fallback = stored.find((entry) => entry.path === item.path);
      return {
        ...fallback,
        ...item,
        pinned: fallback?.pinned ?? item.pinned ?? false,
        template: item.template ?? fallback?.template ?? null,
        preview: item.preview ?? fallback?.preview ?? null,
        error: item.error ?? fallback?.error ?? null,
        last: item.last || fallback?.last || new Date().toISOString(),
      };
    });

    const missing = stored.filter((entry) => !received.some((item) => item.path === entry.path));
    return [...merged, ...missing];
  }

  private async tryNativeCall<T>(label: string, invoke: () => Promise<T | null | undefined> | T | null | undefined): Promise<T | null> {
    if (!this.native) return null;

    try {
      const result = await this.withTimeout(Promise.resolve(invoke()), EngineBridge.NATIVE_TIMEOUT, label);
      return (result ?? null) as T | null;
    } catch (error) {
      console.warn(`[EngineBridge] ${label} call failed, falling back`, error);
      return null;
    }
  }

  private withTimeout<T>(promise: Promise<T | null | undefined>, timeoutMs: number, label: string): Promise<T | null | undefined> {
    if (typeof timeoutMs !== 'number' || timeoutMs <= 0) {
      return promise;
    }

    let timeoutId: ReturnType<typeof setTimeout> | undefined;
    const timeoutPromise = new Promise<never>((_, reject) => {
      timeoutId = setTimeout(() => {
        reject(new Error(`${label} timed out after ${timeoutMs}ms`));
      }, timeoutMs);
    });

    return Promise.race([promise, timeoutPromise]).finally(() => {
      if (timeoutId !== undefined) {
        clearTimeout(timeoutId);
      }
    });
  }
}

export const engine = new EngineBridge();
