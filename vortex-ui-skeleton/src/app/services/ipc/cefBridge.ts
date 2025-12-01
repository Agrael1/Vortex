import type { ProjectSnapshot } from '@state/types';
import type { Operation } from 'fast-json-patch';
import { prepareCommandBatch, serializeCommandBatch, type ProjectCommandBatch } from '@/app/services/ipc/projectCommands';

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
  uid?: string;
  type: string;
  params: Record<string, unknown>;
  pos: [number, number];
  ptr?: number;
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
  width?: number;
  height?: number;
  fps?: number;
  colorSpace?: string;
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
  createProject?: (payload: CreateProjectPayload | string) => Promise<ProjectDTO>;
  ListRecentProjectsAsync?: () => Promise<Recent[]>;
  OpenProjectAsync?: (path: string) => Promise<ProjectDTO>;
  CreateProjectAsync?: (payload: CreateProjectPayload | string) => Promise<ProjectDTO>;
  ShowOpenProjectDialogAsync?: () => Promise<string | null | undefined>;
  ShowSelectFolderDialogAsync?: () => Promise<string | null | undefined>;
  ShowOpenFileDialogAsync?: (optionsJson?: string) => Promise<string | null | undefined> | string | null | undefined;
  saveProject?: (path: string, snapshotJson: string) => Promise<void | boolean> | void | boolean;
  SaveProjectAsync?: (path: string, snapshotJson: string) => Promise<void | boolean> | void | boolean;
  applyProjectPatch?: (path: string, patchJson: string) => Promise<void | boolean> | void | boolean;
  ApplyProjectPatchAsync?: (path: string, patchJson: string) => Promise<void | boolean> | void | boolean;
  persistProject?: (path: string) => Promise<void | boolean> | void | boolean;
  PersistProjectAsync?: (path: string) => Promise<void | boolean> | void | boolean;
  resetProjectState?: (path: string, snapshotJson: string) => Promise<void | boolean> | void | boolean;
  ResetProjectStateAsync?: (path: string, snapshotJson: string) => Promise<void | boolean> | void | boolean;
  submitProjectCommands?: (path: string, batchJson: string) => Promise<void | boolean> | void | boolean;
  SubmitProjectCommandsAsync?: (path: string, batchJson: string) => Promise<void | boolean> | void | boolean;
  configureAutosave?: (enabled: boolean, delayMs: number) => Promise<void | boolean> | void | boolean;
  ConfigureAutosaveAsync?: (enabled: boolean, delayMs: number) => Promise<void | boolean> | void | boolean;
  on?: (event: string, handler: EngineEventHandler) => () => void;
  getNodeProperties?: (nodePtr: number) => Promise<unknown> | unknown;
  GetNodePropertiesAsync?: (nodePtr: number) => Promise<unknown> | unknown;
  getNodeTypes?: () => Promise<unknown> | unknown;
  GetNodeTypesAsync?: () => Promise<unknown> | unknown;
  play?: () => Promise<void | boolean> | void | boolean;
  stop?: () => Promise<void | boolean> | void | boolean;
  Play?: () => Promise<void | boolean> | void | boolean;
  Stop?: () => Promise<void | boolean> | void | boolean;
  minimizeWindow?: () => Promise<void | boolean> | void | boolean;
  MinimizeWindow?: () => Promise<void | boolean> | void | boolean;
  toggleMaximizeWindow?: () => Promise<void | boolean> | void | boolean;
  ToggleMaximizeWindow?: () => Promise<void | boolean> | void | boolean;
  requestExit?: () => Promise<void | boolean> | void | boolean;
  RequestExit?: () => Promise<void | boolean> | void | boolean;
};

const RECENTS_STORAGE_KEY = 'vortex.hub.recents';
const RECENTS_EVENT = 'vortex.recents';
const LAST_PROJECT_STORAGE_KEY = 'vortex.session.lastProject';
const LAST_PROJECT_EVENT = 'vortex.lastProject';
const LOG_EVENT = 'vortex.log';
const NODE_UPDATE_EVENT = 'vortex.node_update';
const NODE_CREATED_EVENT = 'vortex.node_created';
const EDGE_CONNECTED_EVENT = 'vortex.edge_connected';
const EDGE_DISCONNECTED_EVENT = 'vortex.edge_disconnected';
const PROJECT_PERSISTED_EVENT = 'vortex.project_persisted';
const PROJECT_EXTERNAL_CHANGE_EVENT = 'vortex.project_external_change';
const TRANSPORT_STATE_EVENT = 'vortex.transport_state';

export const DEFAULT_AUTOSAVE_DELAY_MS = 1200;
const AUTOSAVE_DELAY_MIN_MS = 250;
const AUTOSAVE_DELAY_MAX_MS = 15000;

const normalizeAutosaveDelay = (value?: number): number => {
  const fallback = DEFAULT_AUTOSAVE_DELAY_MS;
  if (!Number.isFinite(value ?? Number.NaN)) {
    return fallback;
  }
  const rounded = Math.round(value as number);
  if (rounded <= 0) {
    return fallback;
  }
  if (rounded < AUTOSAVE_DELAY_MIN_MS) {
    return AUTOSAVE_DELAY_MIN_MS;
  }
  if (rounded > AUTOSAVE_DELAY_MAX_MS) {
    return AUTOSAVE_DELAY_MAX_MS;
  }
  return rounded;
};

type NodeUpdatePayload = {
  nodePtr: number;
  propIndex: number;
  value: unknown;
};

export type NodeCreatedPayload = {
  ptr: number;
  id: string;
  uid?: string | null;
  type: string;
  label: string;
  position: { x: number; y: number };
  clientNodeId?: string | null;
  props?: Record<string, unknown>;
};

type NodePropertySpec = {
  name?: string;
  index?: number;
  [key: string]: unknown;
};

export type NodePropertySchema = {
  properties?: NodePropertySpec[];
};

export type EdgeConnectionPayload = {
  sourcePtr: number;
  targetPtr: number;
  sourceSlot?: number;
  targetSlot?: number;
  edgeId?: string | null;
};

export type EdgeDisconnectionPayload = {
  sourcePtr?: number;
  targetPtr?: number;
  sourceSlot?: number;
  targetSlot?: number;
  edgeId?: string | null;
};

export type ProjectPersistedPayload = {
  path: string;
  savedAt: string;
  hash?: string | null;
  reason?: string | null;
};

export type ProjectExternalChangePayload = {
  path: string;
  changedAt: string;
};

export type TransportStatus = 'ready' | 'playing' | 'paused' | 'error';

export type TransportStatePayload = {
  state: TransportStatus;
  fps?: number;
  droppedFrames?: number;
  reason?: string | null;
  timestamp?: string | null;
  dropTarget?: string | null;
  dropTargetId?: number | null;
};

export type ConfigureAutosaveOptions = {
  enabled: boolean;
  delayMs?: number;
};

type FileDialogOptions = {
  filters?: string[];
  title?: string;
};

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
    const width = Number.isFinite((parsed as any).width) ? Number((parsed as any).width) : undefined;
    const height = Number.isFinite((parsed as any).height) ? Number((parsed as any).height) : undefined;
    const fps = Number.isFinite((parsed as any).fps) ? Number((parsed as any).fps) : undefined;
    const colorSpace = typeof (parsed as any).colorSpace === 'string' ? (parsed as any).colorSpace : undefined;
    return { name, path, lastOpened, template, width, height, fps, colorSpace };
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
    width: Number.isFinite(entry.width) ? entry.width : undefined,
    height: Number.isFinite(entry.height) ? entry.height : undefined,
    fps: Number.isFinite(entry.fps) ? entry.fps : undefined,
    colorSpace: typeof entry.colorSpace === 'string' ? entry.colorSpace : undefined,
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

const emitNodeUpdate = (payload: NodeUpdatePayload) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(NODE_UPDATE_EVENT, { detail: payload }));
};

const emitNodeCreated = (payload: NodeCreatedPayload) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(NODE_CREATED_EVENT, { detail: payload }));
};

const emitEdgeConnected = (payload: EdgeConnectionPayload) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(EDGE_CONNECTED_EVENT, { detail: payload }));
};

const emitEdgeDisconnected = (payload: EdgeDisconnectionPayload) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(EDGE_DISCONNECTED_EVENT, { detail: payload }));
};

const emitProjectPersisted = (payload: ProjectPersistedPayload) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(PROJECT_PERSISTED_EVENT, { detail: payload }));
};

const emitProjectExternalChange = (payload: ProjectExternalChangePayload) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(PROJECT_EXTERNAL_CHANGE_EVENT, { detail: payload }));
};

const emitTransportState = (payload: TransportStatePayload) => {
  if (typeof window === 'undefined' || typeof window.dispatchEvent !== 'function') return;
  window.dispatchEvent(new CustomEvent(TRANSPORT_STATE_EVENT, { detail: payload }));
};

class EngineBridge {
  private native: NativeLike | null;
  private static readonly NATIVE_TIMEOUT = 4000;
  private static readonly HEAVY_NATIVE_TIMEOUT = 20000;
  private static readonly FILE_DIALOG_TIMEOUT = 120_000;
  private nodeTypesCache: { list: string[]; timestamp: number } | null = null;
  private static readonly NODE_TYPES_TTL_MS = 60_000;

  constructor() {
    this.native = this.resolveNative();
    this.bindGlobalMessages();
  }

  private resolveNative(): NativeLike | null {
    if (typeof window === 'undefined') return null;
    const win = window as unknown as Record<string, unknown>;

    const hasVortexCall = typeof (win as any).vortexCallAsync === 'function';
    if (hasVortexCall) {
      const call = (method: string, ...args: unknown[]) => (win as any).vortexCallAsync(method, ...args);
      const callSync = typeof (win as any).vortexCall === 'function'
        ? (method: string, ...args: unknown[]) => {
            try {
              (win as any).vortexCall(method, ...args);
              return true;
            } catch (error) {
              console.warn(`[EngineBridge] vortexCall ${method} failed`, error);
              return false;
            }
          }
        : null;
      const fireAndForget = (method: string, ...args: unknown[]) => {
        if (callSync) {
          return callSync(method, ...args);
        }
        try {
          const task = call(method, ...args);
          if (task && typeof (task as Promise<unknown>).then === 'function') {
            (task as Promise<unknown>).catch((error) => {
              console.warn(`[EngineBridge] ${method} async call failed`, error);
            });
          }
        } catch (error) {
          console.warn(`[EngineBridge] ${method} dispatch failed`, error);
          return false;
        }
        return true;
      };
      const serializeCreatePayload = (payload: CreateProjectPayload | string) =>
        typeof payload === 'string' ? payload : JSON.stringify(payload);
      return {
        listRecent: () => call('ListRecentProjectsAsync'),
        openProject: (path: string) => call('OpenProjectAsync', path),
        createProject: (payload: CreateProjectPayload | string) => call('CreateProjectAsync', serializeCreatePayload(payload)),
        ShowOpenProjectDialogAsync: () => call('ShowOpenProjectDialogAsync'),
        ShowSelectFolderDialogAsync: () => call('ShowSelectFolderDialogAsync'),
        ShowOpenFileDialogAsync: (optionsJson?: string) => call('ShowOpenFileDialogAsync', optionsJson ?? '{}'),
        getNodeTypes: () => call('GetNodeTypesAsync'),
        GetNodeTypesAsync: () => call('GetNodeTypesAsync'),
        getNodeProperties: (nodePtr: number) => call('GetNodePropertiesAsync', nodePtr),
        GetNodePropertiesAsync: (nodePtr: number) => call('GetNodePropertiesAsync', nodePtr),
        saveProject: (path: string, snapshotJson: string) => call('SaveProjectAsync', path, snapshotJson),
        SaveProjectAsync: (path: string, snapshotJson: string) => call('SaveProjectAsync', path, snapshotJson),
        applyProjectPatch: (path: string, patchJson: string) => call('ApplyProjectPatchAsync', path, patchJson),
        ApplyProjectPatchAsync: (path: string, patchJson: string) => call('ApplyProjectPatchAsync', path, patchJson),
        persistProject: (path: string) => call('PersistProjectAsync', path),
        PersistProjectAsync: (path: string) => call('PersistProjectAsync', path),
        resetProjectState: (path: string, snapshotJson: string) => call('ResetProjectStateAsync', path, snapshotJson),
        ResetProjectStateAsync: (path: string, snapshotJson: string) => call('ResetProjectStateAsync', path, snapshotJson),
        submitProjectCommands: (path: string, batchJson: string) => call('SubmitProjectCommandsAsync', path, batchJson),
        SubmitProjectCommandsAsync: (path: string, batchJson: string) => call('SubmitProjectCommandsAsync', path, batchJson),
        CreateProjectAsync: (payload: CreateProjectPayload | string) => call('CreateProjectAsync', serializeCreatePayload(payload)),
        configureAutosave: (enabled: boolean, delayMs?: number) => call('ConfigureAutosaveAsync', enabled, normalizeAutosaveDelay(delayMs)),
        ConfigureAutosaveAsync: (enabled: boolean, delayMs?: number) => call('ConfigureAutosaveAsync', enabled, normalizeAutosaveDelay(delayMs)),
        play: () => fireAndForget('Play'),
        stop: () => fireAndForget('Stop'),
        minimizeWindow: () => fireAndForget('MinimizeWindow'),
        MinimizeWindow: () => fireAndForget('MinimizeWindow'),
        toggleMaximizeWindow: () => fireAndForget('ToggleMaximizeWindow'),
        ToggleMaximizeWindow: () => fireAndForget('ToggleMaximizeWindow'),
        requestExit: () => fireAndForget('RequestExit'),
        RequestExit: () => fireAndForget('RequestExit'),
      };
    }

    const candidates = [win.VortexHub, win.vortex, win.Vortex];
    for (const candidate of candidates) {
      if (candidate && typeof candidate === 'object') {
        return candidate as NativeLike;
      }
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

    const sanitizedTarget = this.ensureSnapshotFilePath(path, name);

    return {
      version: '1.0',
      name,
      path: sanitizedTarget,
      template: payload?.template ?? null,
      settings: { width, height, fps, colorSpace },
      assets: [],
      graph: { nodes: [], edges: [] },
    };
  }

  private ensureSnapshotFilePath(rawPath: string, fallbackName: string): string {
    const trimmed = rawPath?.trim() ?? '';
    const normalized = trimmed.replace(/[\\/]+$/u, '');
    const segments = normalized.split(/[\\/]/u);
    const leaf = segments[segments.length - 1] ?? '';
    const hasExtension = /\.[^./\\]+$/u.test(leaf);
    if (hasExtension) {
      return normalized.length ? normalized : `${this.safeFileName(fallbackName)}.vortex`;
    }

    if (!normalized.length) {
      return `${this.safeFileName(fallbackName)}.vortex`;
    }

    const baseDir = normalized;
    const separator = baseDir.includes('\\') && !baseDir.includes('/') ? '\\' : '/';
    const safeName = this.safeFileName(fallbackName);
    const dir = baseDir.replace(/[\\/]+$/u, '');
    return `${dir}${dir ? separator : ''}${safeName}.vortex`;
  }

  private safeFileName(name: string): string {
    const cleaned = name.replace(/[^a-zA-Z0-9._-]+/gu, '-').replace(/^-+|-+$/gu, '');
    return cleaned.length ? cleaned : 'project';
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
    const nativeAvailable = Boolean(this.native?.openProject || this.native?.OpenProjectAsync);

    const directProject = await this.tryNativeCall(
      'openProject',
      () => this.native?.openProject?.(normalizedPath),
      EngineBridge.HEAVY_NATIVE_TIMEOUT,
    );

    const asyncProject = directProject?.path
      ? null
      : await this.tryNativeCall(
          'OpenProjectAsync',
          () => this.native?.OpenProjectAsync?.(normalizedPath),
          EngineBridge.HEAVY_NATIVE_TIMEOUT,
        );

    const resolvedProject = directProject ?? asyncProject;
    if (resolvedProject?.path) {
      this.recordRecentFromProject(resolvedProject);
      return resolvedProject;
    }

    if (!nativeAvailable) {
      const stub = this.ensureProjectStub(normalizedPath);
      this.recordRecentFromProject(stub);
      return stub;
    }

    throw new Error('Native openProject call did not return a project path.');
  }

  async createProject(payload: CreateProjectPayload): Promise<ProjectDTO> {
    const location = payload.location?.trim();
    if (!location) {
      throw new Error('Please specify a target location for the project');
    }

    const nativeAvailable = Boolean(this.native?.createProject || this.native?.CreateProjectAsync);

    const direct = await this.tryNativeCall(
      'createProject',
      () => this.native?.createProject?.(payload),
      EngineBridge.HEAVY_NATIVE_TIMEOUT,
    );
    const asyncResult = direct?.path
      ? null
      : await this.tryNativeCall(
          'CreateProjectAsync',
          () => this.native?.CreateProjectAsync?.(payload),
          EngineBridge.HEAVY_NATIVE_TIMEOUT,
        );

    const resolvedProject = direct ?? asyncResult;
    if (resolvedProject?.path) {
      this.recordRecentFromProject(resolvedProject, payload.template ?? null);
      return resolvedProject;
    }

    if (!nativeAvailable) {
      const stub = this.ensureProjectStub(location, payload);
      this.recordRecentFromProject(stub, payload.template ?? null);
      return stub;
    }

    throw new Error('Native createProject call did not return a project path.');
  }

  async getNodeProperties(nodePtr: number): Promise<NodePropertySchema> {
    if (!Number.isFinite(nodePtr)) {
      return { properties: [] };
    }

    const direct = await this.tryNativeCall('getNodeProperties', () => this.native?.getNodeProperties?.(nodePtr));
    if (direct) {
      return this.normalizePropertySchema(direct);
    }

    const asyncResult = await this.tryNativeCall('GetNodePropertiesAsync', () => this.native?.GetNodePropertiesAsync?.(nodePtr));
    if (asyncResult) {
      return this.normalizePropertySchema(asyncResult);
    }

    return { properties: [] };
  }

  async play(): Promise<void> {
    if (await this.callNativeVoid('play')) {
      this.emitLocalTransportState('playing');
      return;
    }

    if (await this.callNativeVoid('Play')) {
      this.emitLocalTransportState('playing');
      return;
    }

    this.emitLocalTransportState('error', { reason: 'Play is not available in the current environment' });
    throw new Error('Play is not available in the current environment');
  }

  async stop(): Promise<void> {
    if (await this.callNativeVoid('stop')) {
      this.emitLocalTransportState('paused');
      return;
    }

    if (await this.callNativeVoid('Stop')) {
      this.emitLocalTransportState('paused');
      return;
    }

    this.emitLocalTransportState('error', { reason: 'Stop is not available in the current environment' });
    throw new Error('Stop is not available in the current environment');
  }

  async minimizeWindow(): Promise<void> {
    if (await this.callNativeVoid('minimizeWindow')) {
      return;
    }

    if (await this.callNativeVoid('MinimizeWindow')) {
      return;
    }

    throw new Error('Window minimization is not available in the current environment');
  }

  async toggleMaximizeWindow(): Promise<void> {
    if (await this.callNativeVoid('toggleMaximizeWindow')) {
      return;
    }

    if (await this.callNativeVoid('ToggleMaximizeWindow')) {
      return;
    }

    throw new Error('Window maximize toggle is not available in the current environment');
  }

  async requestExit(): Promise<void> {
    if (await this.callNativeVoid('requestExit')) {
      return;
    }

    if (await this.callNativeVoid('RequestExit')) {
      return;
    }

    if (typeof window !== 'undefined') {
      window.close();
      return;
    }

    throw new Error('Exit is not available in the current environment');
  }

  async listNodeTypes(forceRefresh = false): Promise<string[]> {
    const cache = this.nodeTypesCache;
    const now = Date.now();
    if (!forceRefresh && cache && now - cache.timestamp < EngineBridge.NODE_TYPES_TTL_MS) {
      return cache.list;
    }

    const fetched = await this.fetchNodeTypes();
    if (fetched.length) {
      this.nodeTypesCache = { list: fetched, timestamp: now };
      return fetched;
    }

    return cache?.list ?? [];
  }

  async browseForProject(): Promise<string | null> {
    if (this.native?.ShowOpenProjectDialogAsync) {
      const selected = await this.tryNativeCall(
        'ShowOpenProjectDialogAsync',
        () => this.native?.ShowOpenProjectDialogAsync?.(),
        EngineBridge.FILE_DIALOG_TIMEOUT,
      );
      return this.normalizeSelectedPath(selected);
    }

    if (typeof window !== 'undefined') {
      const fallback = window.prompt('Enter absolute path to a Vortex project (*.vortex)');
      const normalized = this.normalizeSelectedPath(fallback);
      if (normalized) {
        return normalized;
      }
    }

    return null;
  }

  async browseForFolder(): Promise<string | null> {
    if (this.native?.ShowSelectFolderDialogAsync) {
      const selected = await this.tryNativeCall(
        'ShowSelectFolderDialogAsync',
        () => this.native?.ShowSelectFolderDialogAsync?.(),
        EngineBridge.FILE_DIALOG_TIMEOUT,
      );
      return this.normalizeSelectedPath(selected);
    }

    if (typeof window !== 'undefined') {
      const fallback = window.prompt('Enter destination folder path');
      const normalized = this.normalizeSelectedPath(fallback);
      if (normalized) {
        return normalized;
      }
    }

    return null;
  }

  async browseForAsset(options?: FileDialogOptions): Promise<string | null> {
    const normalizedFilters = (options?.filters ?? [])
      .map((filter) => (typeof filter === 'string' ? filter.trim() : ''))
      .filter((filter) => filter.length > 0);
    const dialogTitle = typeof options?.title === 'string' ? options.title.trim() : '';
    const payloadNeeded = normalizedFilters.length > 0 || dialogTitle.length > 0;
    const payload = payloadNeeded ? JSON.stringify({ filters: normalizedFilters, title: dialogTitle || undefined }) : '{}';

    if (this.native?.ShowOpenFileDialogAsync) {
      const selected = await this.tryNativeCall(
        'ShowOpenFileDialogAsync',
        () => this.native?.ShowOpenFileDialogAsync?.(payload),
        EngineBridge.FILE_DIALOG_TIMEOUT,
      );
      return this.normalizeSelectedPath(selected);
    }

    if (typeof window !== 'undefined') {
      const fallbackMessage = dialogTitle.length ? `${dialogTitle} (enter full path)` : 'Enter absolute file path';
      const fallback = window.prompt(fallbackMessage);
      const normalized = this.normalizeSelectedPath(fallback);
      if (normalized) {
        return normalized;
      }
    }

    return null;
  }

  async saveProject(snapshot?: ProjectSnapshot): Promise<'native' | 'fallback'> {
    if (!snapshot || !snapshot.path || !snapshot.meta || !snapshot.settings || !snapshot.graph) {
      throw new Error('Cannot save project without a complete snapshot and path');
    }

    const targetPath = snapshot.path.trim();
    if (!targetPath.length) {
      throw new Error('Project path is empty and cannot be saved');
    }

    const serialized = JSON.stringify(snapshot, null, 2);
    const attempt = async (label: 'saveProject' | 'SaveProjectAsync'): Promise<boolean> => {
      if (!this.native) return false;

      const method = label === 'saveProject' ? this.native.saveProject : this.native.SaveProjectAsync;
      if (typeof method !== 'function') return false;

      try {
        const result = await this.withTimeout(
          Promise.resolve(method.call(this.native, targetPath, serialized)),
          EngineBridge.NATIVE_TIMEOUT,
          label,
        );
        return result !== false;
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
        storage.setItem('vortex.editor.lastSnapshot', serialized);
        storage.setItem('vortex.editor.lastSnapshotPath', targetPath);
        console.warn('[EngineBridge] Using fallback save stub – native save not available');
        return 'fallback';
      } catch (error) {
        console.warn('[EngineBridge] Fallback save stub failed', error);
      }
    }

    throw new Error('Save project is not available in the current environment');
  }

  async applyProjectPatch(path: string, patch: Operation[]): Promise<void> {
    if (!Array.isArray(patch) || !patch.length) {
      return;
    }

    const targetPath = path?.trim();
    if (!targetPath?.length) {
      throw new Error('Project path must be provided when applying a patch');
    }

    const serialized = JSON.stringify(patch);
    const attempt = async (label: 'applyProjectPatch' | 'ApplyProjectPatchAsync'): Promise<boolean> => {
      if (!this.native) return false;
      const method = label === 'applyProjectPatch' ? this.native?.applyProjectPatch : this.native?.ApplyProjectPatchAsync;
      if (typeof method !== 'function') return false;

      try {
        const result = await this.withTimeout(
          Promise.resolve(method.call(this.native, targetPath, serialized)),
          EngineBridge.NATIVE_TIMEOUT,
          label,
        );
        return result !== false;
      } catch (error) {
        console.warn(`[EngineBridge] ${label} call failed`, error);
        return false;
      }
    };

    if (await attempt('applyProjectPatch')) {
      return;
    }

    if (await attempt('ApplyProjectPatchAsync')) {
      return;
    }

    throw new Error('ApplyProjectPatch is not available in the current environment');
  }

  async submitProjectCommands(batch: ProjectCommandBatch): Promise<void> {
    const prepared = prepareCommandBatch(batch);
    const serialized = serializeCommandBatch(prepared);

    const attempt = async (
      label: 'submitProjectCommands' | 'SubmitProjectCommandsAsync',
    ): Promise<boolean> => {
      if (!this.native) return false;
      const method =
        label === 'submitProjectCommands'
          ? this.native?.submitProjectCommands
          : this.native?.SubmitProjectCommandsAsync;
      if (typeof method !== 'function') return false;

      try {
        const timeout = label.toLowerCase().includes('submitprojectcommands')
          ? EngineBridge.HEAVY_NATIVE_TIMEOUT
          : EngineBridge.NATIVE_TIMEOUT;
        const result = await this.withTimeout(
          Promise.resolve(method.call(this.native, prepared.projectPath, serialized)),
          timeout,
          label,
        );
        return result !== false;
      } catch (error) {
        console.warn(`[EngineBridge] ${label} call failed`, error);
        return false;
      }
    };

    if (await attempt('submitProjectCommands')) {
      return;
    }

    if (await attempt('SubmitProjectCommandsAsync')) {
      return;
    }

    throw new Error('SubmitProjectCommands is not available in the current environment');
  }

  async persistProject(path: string): Promise<void> {
    const targetPath = path?.trim();
    if (!targetPath?.length) {
      throw new Error('Project path must be provided when persisting');
    }

    const attempt = async (label: 'persistProject' | 'PersistProjectAsync'): Promise<boolean> => {
      if (!this.native) return false;
      const method = label === 'persistProject' ? this.native?.persistProject : this.native?.PersistProjectAsync;
      if (typeof method !== 'function') return false;

      try {
        const result = await this.withTimeout(
          Promise.resolve(method.call(this.native, targetPath)),
          EngineBridge.HEAVY_NATIVE_TIMEOUT,
          label,
        );
        return result !== false;
      } catch (error) {
        console.warn(`[EngineBridge] ${label} call failed`, error);
        return false;
      }
    };

    if (await attempt('persistProject')) {
      return;
    }

    if (await attempt('PersistProjectAsync')) {
      return;
    }

    throw new Error('PersistProject is not available in the current environment');
  }

  async resetProjectState(snapshot: ProjectSnapshot): Promise<void> {
    if (!snapshot?.path?.trim()) {
      throw new Error('Cannot reset project state without a project path');
    }

    const targetPath = snapshot.path.trim();
    const serialized = JSON.stringify(snapshot, null, 2);
    const attempt = async (label: 'resetProjectState' | 'ResetProjectStateAsync'): Promise<boolean> => {
      if (!this.native) return false;
      const method = label === 'resetProjectState' ? this.native?.resetProjectState : this.native?.ResetProjectStateAsync;
      if (typeof method !== 'function') return false;

      try {
        const result = await this.withTimeout(
          Promise.resolve(method.call(this.native, targetPath, serialized)),
          EngineBridge.NATIVE_TIMEOUT,
          label,
        );
        return result !== false;
      } catch (error) {
        console.warn(`[EngineBridge] ${label} call failed`, error);
        return false;
      }
    };

    if (await attempt('resetProjectState')) {
      return;
    }

    if (await attempt('ResetProjectStateAsync')) {
      return;
    }

    throw new Error('ResetProjectState is not available in the current environment');
  }

  async configureAutosave(options: ConfigureAutosaveOptions): Promise<void> {
    const enabled = Boolean(options?.enabled);
    const delay = normalizeAutosaveDelay(options?.delayMs ?? DEFAULT_AUTOSAVE_DELAY_MS);

    const attempt = async (label: 'configureAutosave' | 'ConfigureAutosaveAsync'): Promise<boolean> => {
      if (!this.native) return false;
      const method = label === 'configureAutosave' ? this.native?.configureAutosave : this.native?.ConfigureAutosaveAsync;
      if (typeof method !== 'function') return false;

      try {
        const result = await this.withTimeout(
          Promise.resolve(method.call(this.native, enabled, delay)),
          EngineBridge.NATIVE_TIMEOUT,
          label,
        );
        return result !== false;
      } catch (error) {
        console.warn(`[EngineBridge] ${label} call failed`, error);
        return false;
      }
    };

    if (await attempt('configureAutosave')) {
      return;
    }

    if (await attempt('ConfigureAutosaveAsync')) {
      return;
    }

    throw new Error('ConfigureAutosave is not available in the current environment');
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

    if (event === 'node:update' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(NODE_UPDATE_EVENT, listener as EventListener);
      return () => window.removeEventListener(NODE_UPDATE_EVENT, listener as EventListener);
    }

    if (event === 'node:created' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(NODE_CREATED_EVENT, listener as EventListener);
      return () => window.removeEventListener(NODE_CREATED_EVENT, listener as EventListener);
    }

    if (event === 'edge:connected' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(EDGE_CONNECTED_EVENT, listener as EventListener);
      return () => window.removeEventListener(EDGE_CONNECTED_EVENT, listener as EventListener);
    }

    if (event === 'edge:disconnected' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(EDGE_DISCONNECTED_EVENT, listener as EventListener);
      return () => window.removeEventListener(EDGE_DISCONNECTED_EVENT, listener as EventListener);
    }

    if (event === 'project:persisted' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(PROJECT_PERSISTED_EVENT, listener as EventListener);
      return () => window.removeEventListener(PROJECT_PERSISTED_EVENT, listener as EventListener);
    }

    if (event === 'project:externalChange' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(PROJECT_EXTERNAL_CHANGE_EVENT, listener as EventListener);
      return () => window.removeEventListener(PROJECT_EXTERNAL_CHANGE_EVENT, listener as EventListener);
    }

    if (event === 'transport:state' && typeof window !== 'undefined') {
      const listener = (e: CustomEvent<T>) => {
        handler(e.detail);
      };
      window.addEventListener(TRANSPORT_STATE_EVENT, listener as EventListener);
      return () => window.removeEventListener(TRANSPORT_STATE_EVENT, listener as EventListener);
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

  private async tryNativeCall<T>(
    label: string,
    invoke: () => Promise<T | null | undefined> | T | null | undefined,
    timeoutOverrideMs?: number,
  ): Promise<T | null> {
    if (!this.native) return null;

    try {
      const timeout = timeoutOverrideMs ?? EngineBridge.NATIVE_TIMEOUT;
      const result = await this.withTimeout(Promise.resolve(invoke()), timeout, label);
      return (result ?? null) as T | null;
    } catch (error) {
      console.warn(`[EngineBridge] ${label} call failed, falling back`, error);
      return null;
    }
  }

  private recordRecentFromProject(project: ProjectDTO, templateOverride?: string | null) {
    if (!project?.path) return;

    this.touchRecents({
      name: project.name,
      path: project.path,
      last: new Date().toISOString(),
      template: templateOverride ?? project.template ?? null,
      width: project.settings?.width,
      height: project.settings?.height,
      fps: project.settings?.fps,
      colorSpace: project.settings?.colorSpace,
    });
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

  private parseLooseJson(raw: string): unknown {
    const trimmed = raw?.trim();
    if (!trimmed) {
      return null;
    }

    try {
      return JSON.parse(trimmed);
    } catch {
      /* fall through */
    }

    const relaxed = trimmed
      .replace(/([,{]\s*)([A-Za-z_][A-Za-z0-9_\-.]*)\s*:/g, '$1"$2":')
      .replace(/'([^']*)'/g, '"$1"')
      .replace(/,\s*([}\]])/g, '$1');

    try {
      return JSON.parse(relaxed);
    } catch (error) {
      console.warn('[EngineBridge] Failed to parse property payload', error);
      return null;
    }
  }

  private inferPropertyType(value: unknown): string {
    if (typeof value === 'boolean') return 'bool';
    if (typeof value === 'number') return Number.isInteger(value) ? 'int' : 'float';
    if (typeof value === 'string') return 'string';
    if (Array.isArray(value)) {
      if (value.length === 2) return 'vec2';
      if (value.length === 3) return 'vec3';
      if (value.length === 4) return 'vec4';
      return 'string';
    }
    if (value && typeof value === 'object') return 'string';
    return 'string';
  }

  private buildSchemaFromDictionary(dict: Record<string, unknown>): NodePropertySchema {
    const entries = Object.entries(dict ?? {});
    const properties = entries
      .map(([name, rawValue], fallbackIndex) => {
        if (!name) {
          return null;
        }

        if (rawValue && typeof rawValue === 'object' && !Array.isArray(rawValue)) {
          const candidate = rawValue as Record<string, unknown>;
          const explicitIndex = typeof candidate.index === 'number' ? candidate.index : fallbackIndex;
          const value = Object.prototype.hasOwnProperty.call(candidate, 'value') ? candidate.value : rawValue;
          return {
            ...candidate,
            name,
            label: typeof candidate.label === 'string' ? candidate.label : name,
            index: explicitIndex,
            value,
            type: typeof candidate.type === 'string' ? candidate.type : this.inferPropertyType(value),
          } as NodePropertySpec;
        }

        return {
          name,
          label: name,
          index: fallbackIndex,
          value: rawValue,
          type: this.inferPropertyType(rawValue),
        } as NodePropertySpec;
      })
      .filter((prop): prop is NodePropertySpec => Boolean(prop));

    return { properties };
  }

  private coercePropertySchema(raw: unknown): NodePropertySchema {
    if (!raw) {
      return { properties: [] };
    }

    if (typeof raw === 'string') {
      const parsed = this.parseLooseJson(raw);
      if (!parsed) {
        return { properties: [] };
      }
      return this.coercePropertySchema(parsed);
    }

    if (typeof raw !== 'object') {
      return { properties: [] };
    }

    const candidate = raw as NodePropertySchema;
    if (Array.isArray(candidate.properties)) {
      return { properties: candidate.properties.slice() };
    }

    return this.buildSchemaFromDictionary(raw as Record<string, unknown>);
  }

  private normalizePropertySchema(raw: unknown): NodePropertySchema {
    const coerced = this.coercePropertySchema(raw);
    if (!coerced.properties?.length) {
      return { properties: [] };
    }

    const normalized = coerced.properties
      .map((prop, fallbackIndex) => {
        if (!prop || typeof prop.name !== 'string') {
          return null;
        }

        const index = Number.isFinite(prop.index) ? Number(prop.index) : fallbackIndex;
        const value = Object.prototype.hasOwnProperty.call(prop, 'value') ? (prop as any).value : undefined;
        const label = typeof (prop as any).label === 'string' ? (prop as any).label : prop.name;
        const type = typeof (prop as any).type === 'string' ? (prop as any).type : this.inferPropertyType(value);

        return {
          ...prop,
          name: prop.name,
          label,
          index,
          type,
        } as NodePropertySpec;
      })
      .filter((prop): prop is NodePropertySpec => Boolean(prop));

    return { properties: normalized };
  }

  private normalizeSelectedPath(candidate: unknown): string | null {
    if (typeof candidate !== 'string') {
      return null;
    }
    const trimmed = candidate.trim();
    return trimmed.length ? trimmed : null;
  }

  private async callNativeVoid(methodName: keyof NativeLike, timeoutOverrideMs?: number, args: unknown[] = []): Promise<boolean> {
    if (!this.native) {
      return false;
    }

    const candidate = this.native?.[methodName];
    if (typeof candidate !== 'function') {
      return false;
    }

    try {
      const fn = candidate as (...fnArgs: unknown[]) => unknown;
      await this.withTimeout(
        Promise.resolve(fn.call(this.native, ...args)),
        timeoutOverrideMs ?? EngineBridge.NATIVE_TIMEOUT,
        String(methodName),
      );
      return true;
    } catch (error) {
      console.warn(`[EngineBridge] ${String(methodName)} call failed`, error);
      return false;
    }
  }

  private normalizeNodeTypeList(raw: unknown): string[] {
    if (!raw) {
      return [];
    }

    if (Array.isArray(raw)) {
      return raw.map((item) => String(item ?? '').trim()).filter((item) => item.length > 0);
    }

    if (typeof raw === 'object') {
      const candidate = raw as Record<string, unknown> & { types?: unknown };
      if (Array.isArray(candidate.types)) {
        return candidate.types.map((item) => String(item ?? '').trim()).filter((item) => item.length > 0);
      }
      return Object.keys(candidate).map((key) => key.trim()).filter((key) => key.length > 0);
    }

    if (typeof raw === 'string') {
      const trimmed = raw.trim();
      if (!trimmed.length) {
        return [];
      }
      try {
        return this.normalizeNodeTypeList(JSON.parse(trimmed));
      } catch {
        return trimmed.split(/[,\s]+/u).map((item) => item.trim()).filter((item) => item.length > 0);
      }
    }

    return [];
  }

  private async fetchNodeTypes(): Promise<string[]> {
    const direct = await this.tryNativeCall('getNodeTypes', () => this.native?.getNodeTypes?.());
    const parsedDirect = this.normalizeNodeTypeList(direct);
    if (parsedDirect.length) {
      return parsedDirect;
    }

    const asyncResult = await this.tryNativeCall('GetNodeTypesAsync', () => this.native?.GetNodeTypesAsync?.());
    const parsedAsync = this.normalizeNodeTypeList(asyncResult);
    if (parsedAsync.length) {
      return parsedAsync;
    }

    return [];
  }
  private emitLocalTransportState(state: TransportStatus, extras?: Partial<TransportStatePayload>) {
    emitTransportState({ state, timestamp: new Date().toISOString(), ...extras });
  }

  private normalizeTransportState(raw: unknown): TransportStatePayload {
    if (Array.isArray(raw) && raw.length) {
      const [stateRaw, fpsRaw, reasonRaw, timestampRaw, droppedRaw, dropTargetRaw, dropTargetIdRaw] = raw;
      return {
        state: this.normalizeTransportStatus(stateRaw),
        fps: this.extractNumeric(fpsRaw),
        reason: this.extractText(reasonRaw),
        timestamp: this.extractText(timestampRaw),
        droppedFrames: this.extractNumeric(droppedRaw),
        dropTarget: this.extractText(dropTargetRaw),
        dropTargetId: this.extractNumeric(dropTargetIdRaw),
      };
    }

    if (raw && typeof raw === 'object') {
      const data = raw as Record<string, unknown>;
      const stateCandidate = data.state ?? data.status;
      return {
        state: this.normalizeTransportStatus(stateCandidate),
        fps: this.extractNumeric(data.fps ?? data.frameRate ?? data.framesPerSecond),
        reason: this.extractText(data.reason ?? data.message),
        timestamp: this.extractText(data.timestamp ?? data.time ?? data.updatedAt),
        droppedFrames: this.extractNumeric(data.droppedFrames ?? data.dropped ?? data.droppedFrameCount),
        dropTarget: this.extractText(data.dropTarget ?? data.dropHint ?? data.lastDropOutput),
        dropTargetId: this.extractNumeric(data.dropTargetId ?? data.dropOutputPtr ?? data.outputPointer),
      };
    }

    if (typeof raw === 'string') {
      return { state: this.normalizeTransportStatus(raw) };
    }

    return { state: 'ready' };
  }

  private normalizeTransportStatus(input: unknown): TransportStatus {
    if (typeof input === 'string') {
      const normalized = input.trim().toLowerCase();
      if (normalized === 'playing') return 'playing';
      if (normalized === 'paused') return 'paused';
      if (normalized === 'stopped' || normalized === 'stop') return 'paused';
      if (normalized === 'error' || normalized === 'failed') return 'error';
      if (normalized === 'ready' || normalized === 'idle' || normalized === 'initialized') return 'ready';
    }

    return 'ready';
  }

  private extractNumeric(value: unknown): number | undefined {
    if (typeof value === 'number' && Number.isFinite(value)) {
      return value;
    }

    if (typeof value === 'string' && value.trim().length) {
      const parsed = Number(value.trim());
      if (Number.isFinite(parsed)) {
        return parsed;
      }
    }

    return undefined;
  }

  private extractText(value: unknown): string | null {
    if (typeof value === 'string') {
      const trimmed = value.trim();
      return trimmed.length ? trimmed : null;
    }

    return null;
  }

  private bindGlobalMessages() {
    if (typeof window === 'undefined') {
      return;
    }

    const handler = (ev: CustomEvent) => {
      const detail = ev.detail as { name?: string; args?: unknown[] };
      if (!detail || typeof detail !== 'object') {
        return;
      }

      if (detail.name === 'node_update') {
        if (!Array.isArray(detail.args) || detail.args.length < 3) {
          return;
        }

        const nodePtr = Number(detail.args[0]);
        const propIndex = Number(detail.args[1]);
        let value: unknown = detail.args[2];
        if (typeof value === 'string') {
          try {
            value = JSON.parse(value);
          } catch {
            /* keep raw string */
          }
        }

        if (!Number.isFinite(nodePtr) || !Number.isFinite(propIndex)) {
          return;
        }

        emitNodeUpdate({ nodePtr, propIndex, value });
        return;
      }

      if (detail.name === 'graph_node_created') {
        const args = Array.isArray(detail.args) ? detail.args : [];
        const ptr = Number(args[0]);
        if (!Number.isFinite(ptr)) {
          return;
        }

        const idCandidate = typeof args[1] === 'string' ? args[1].trim() : '';
        const typeCandidate = typeof args[2] === 'string' ? args[2].trim() : '';
        const labelCandidate = typeof args[3] === 'string' ? args[3].trim() : '';
        const x = Number(args[4]);
        const y = Number(args[5]);
        const clientNodeIdCandidate = typeof args[6] === 'string' ? args[6].trim() : '';
        const propsJson = typeof args[7] === 'string' ? args[7] : null;
        const uidCandidate = typeof args[8] === 'string' ? args[8].trim() : '';

        let props: Record<string, unknown> | undefined;
        if (propsJson && propsJson.trim().length) {
          try {
            props = JSON.parse(propsJson);
          } catch (error) {
            console.warn('[EngineBridge] Failed to parse props for node creation event', error);
          }
        }

        emitNodeCreated({
          ptr,
          id: idCandidate || clientNodeIdCandidate || `node-${ptr}`,
          type: typeCandidate || 'Node',
          label: labelCandidate || idCandidate || clientNodeIdCandidate || `Node ${ptr}`,
          position: {
            x: Number.isFinite(x) ? x : 0,
            y: Number.isFinite(y) ? y : 0,
          },
          clientNodeId: clientNodeIdCandidate || null,
          uid: uidCandidate || null,
          props,
        });
        return;
      }

      if (detail.name === 'edge_connected') {
        const args = Array.isArray(detail.args) ? detail.args : [];
        const sourcePtr = Number(args[0]);
        const targetPtr = Number(args[1]);
        if (!Number.isFinite(sourcePtr) || !Number.isFinite(targetPtr)) {
          return;
        }

        const edgeIdCandidate = args[2];
        const sourceSlotCandidate = args[3];
        const targetSlotCandidate = args[4];

        const payload: EdgeConnectionPayload = {
          sourcePtr,
          targetPtr,
        };

        if (typeof edgeIdCandidate === 'string' && edgeIdCandidate.trim().length) {
          payload.edgeId = edgeIdCandidate.trim();
        }

        const sourceSlot = Number(sourceSlotCandidate);
        if (Number.isFinite(sourceSlot)) {
          payload.sourceSlot = sourceSlot;
        }

        const targetSlot = Number(targetSlotCandidate);
        if (Number.isFinite(targetSlot)) {
          payload.targetSlot = targetSlot;
        }

        emitEdgeConnected(payload);
        return;
      }

      if (detail.name === 'edge_disconnected') {
        const args = Array.isArray(detail.args) ? detail.args : [];
        const payload: EdgeDisconnectionPayload = {};

        const sourcePtr = Number(args[0]);
        if (Number.isFinite(sourcePtr)) {
          payload.sourcePtr = sourcePtr;
        }

        const targetPtr = Number(args[1]);
        if (Number.isFinite(targetPtr)) {
          payload.targetPtr = targetPtr;
        }

        const edgeIdCandidate = args[2];
        if (typeof edgeIdCandidate === 'string' && edgeIdCandidate.trim().length) {
          payload.edgeId = edgeIdCandidate.trim();
        }

        const sourceSlot = Number(args[3]);
        if (Number.isFinite(sourceSlot)) {
          payload.sourceSlot = sourceSlot;
        }

        const targetSlot = Number(args[4]);
        if (Number.isFinite(targetSlot)) {
          payload.targetSlot = targetSlot;
        }

        if (!payload.edgeId && payload.sourcePtr == null && payload.targetPtr == null) {
          return;
        }

        emitEdgeDisconnected(payload);
        return;
      }

      if (detail.name === 'project_persisted') {
        const args = Array.isArray(detail.args) ? detail.args : [];
        const path = typeof args[0] === 'string' ? args[0] : null;
        const savedAt = typeof args[1] === 'string' ? args[1] : new Date().toISOString();
        const hash = typeof args[2] === 'string' ? args[2] : null;
        const reason = typeof args[3] === 'string' ? args[3] : null;
        if (!path) {
          return;
        }

        emitProjectPersisted({ path, savedAt, hash, reason });
        return;
      }

      if (detail.name === 'project_external_change') {
        const args = Array.isArray(detail.args) ? detail.args : [];
        const path = typeof args[0] === 'string' ? args[0] : null;
        const changedAt = typeof args[1] === 'string' ? args[1] : new Date().toISOString();
        if (!path) {
          return;
        }

        emitProjectExternalChange({ path, changedAt });
        return;
      }

      if (detail.name === 'transport_state') {
        const payload = this.normalizeTransportState(detail.args);
        emitTransportState(payload);
        return;
      }
    };

    window.addEventListener('cef-message', handler as EventListener);
  }
}

export const engine = new EngineBridge();
