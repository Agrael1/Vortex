import { ChangeEvent, FormEvent, KeyboardEvent, ReactNode, useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { useLocation, useNavigate } from 'react-router-dom';
import { CreateProjectModal } from '@/app/components/CreateProjectModal';
import { engine, type CreateProjectPayload, type Recent, type LastProject } from '@/app/services/ipc/cefBridge';
import { useProjectCommands } from '@state/hooks/useProjectCommands';
import { useNotificationCenter } from '@state/hooks/useNotificationCenter';
import {
  SPLASH_AUTO_CONTINUE_KEY,
  SPLASH_AUTO_DELAY_PRESETS,
  SPLASH_FALLBACK_DELAY_PRESETS,
  buildDelayOptions,
  clampAutoDelay,
  clampFallbackDelay,
  readAutoDelayPreference,
  readFallbackDelayPreference,
  writeAutoDelayPreference,
  writeFallbackDelayPreference,
} from '@/app/constants/preferences';
import {
  compareRecents,
  deriveNameFromPath,
  formatLastUsed,
  loadRecentsFromStorage,
  mergeRecentCollections,
  persistRecents,
} from './helpers/recents';
import { recentsReducer, type RecentsAction } from './helpers/recentsReducer';

const escapeRegExp = (value: string) => value.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');

const highlightMatches = (text: string, query: string): ReactNode => {
  const normalized = query.trim();
  if (!normalized.length) {
    return text;
  }
  const escaped = escapeRegExp(normalized);
  if (!escaped.length) {
    return text;
  }
  const regex = new RegExp(`(${escaped})`, 'gi');
  const parts = text.split(regex);
  if (parts.length === 1) {
    return text;
  }
  const lowered = normalized.toLowerCase();
  return parts.map((part, index) => {
    if (!part.length) {
      return null;
    }
    if (part.toLowerCase() === lowered) {
      return (
        <mark key={`match-${index}`} className="rounded bg-blue-500/30 px-0.5 text-inherit" data-testid="search-highlight">
          {part}
        </mark>
      );
    }
    return (
      <span key={`text-${index}`}>{part}</span>
    );
  });
};

export type TemplateSpec = {
  name: string;
  description: string;
  icon: string;
  accent: string;
  preset: { width: number; height: number; fps: number; colorSpace: string };
};

export type CreateFormState = {
  name: string;
  location: string;
  width: string;
  height: string;
  fps: string;
  colorSpace: string;
};

const TEMPLATES: TemplateSpec[] = [
  {
    name: 'Blank Project',
    description: 'Start from a clean canvas',
    icon: '🟦',
    accent: 'bg-sky-500/20 text-sky-300',
    preset: { width: 1920, height: 1080, fps: 60, colorSpace: 'Rec.709' },
  },
  {
    name: 'Live Stream',
    description: 'Streaming setup with overlays and chat inputs',
    icon: '📡',
    accent: 'bg-purple-500/20 text-purple-300',
    preset: { width: 1920, height: 1080, fps: 60, colorSpace: 'Rec.709' },
  },
  {
    name: 'NDI Bridge',
    description: 'Route live video feeds over NDI',
    icon: '🖧',
    accent: 'bg-blue-500/20 text-blue-300',
    preset: { width: 1920, height: 1080, fps: 50, colorSpace: 'Rec.709' },
  },
  {
    name: 'Video Processor',
    description: 'Default effects and color grading pipeline',
    icon: '🎚️',
    accent: 'bg-emerald-500/20 text-emerald-300',
    preset: { width: 3840, height: 2160, fps: 30, colorSpace: 'Rec.2020' },
  },
];

const DEFAULT_FORM: CreateFormState = {
  name: '',
  location: '',
  width: '1920',
  height: '1080',
  fps: '60',
  colorSpace: 'Rec.709',
};


const BUTTON_BASE =
  'inline-flex items-center gap-2 rounded-lg border px-4 py-2 text-sm font-medium transition focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-sky-400/60 focus-visible:ring-offset-0';
const ACCENT_BUTTON = `${BUTTON_BASE} bg-gradient-to-r from-sky-500/80 to-indigo-500/80 text-white border-transparent shadow-lg shadow-sky-900/40 hover:from-sky-400/80 hover:to-indigo-400/80`;
const SECONDARY_BUTTON = `${BUTTON_BASE} border-ui-border bg-black/40 text-gray-100 hover:border-sky-500/60 hover:text-sky-200`;
const GHOST_BUTTON = `${BUTTON_BASE} border-transparent bg-white/5 text-gray-200 hover:bg-white/10`;
const SUBTLE_BUTTON = `${BUTTON_BASE} border-transparent text-gray-400 hover:text-gray-100 px-0`;

export function Hub() {
  const nav = useNavigate();
  const location = useLocation();
  const [recents, setRecents] = useState<Recent[]>(() => loadRecentsFromStorage());
  const [isLoading, setIsLoading] = useState(true);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [busyPath, setBusyPath] = useState<string | null>(null);
  const [actionError, setActionError] = useState<string | null>(null);
  const [searchTerm, setSearchTerm] = useState('');
  const [activeMenuPath, setActiveMenuPath] = useState<string | null>(null);
  const [lastProject, setLastProject] = useState<LastProject | null>(() => engine.getLastProject());
  const [autoContinue, setAutoContinue] = useState(() => {
    if (typeof window === 'undefined') return false;
    return window.localStorage.getItem(SPLASH_AUTO_CONTINUE_KEY) === 'true';
  });
  const [autoDelay, setAutoDelay] = useState(() => readAutoDelayPreference());
  const [fallbackDelay, setFallbackDelay] = useState(() => readFallbackDelayPreference());

  const [isCreateOpen, setIsCreateOpen] = useState(false);
  const [isCreating, setIsCreating] = useState(false);
  const [createError, setCreateError] = useState<string | null>(null);
  const [selectedTemplate, setSelectedTemplate] = useState<TemplateSpec | null>(null);
  const [form, setForm] = useState<CreateFormState>(DEFAULT_FORM);
  const [renameTarget, setRenameTarget] = useState<{ path: string; name: string } | null>(null);
  const [renameValue, setRenameValue] = useState('');
  const [focusedIndex, setFocusedIndex] = useState(-1);
  const listRef = useRef<HTMLDivElement | null>(null);
  const { loadProject } = useProjectCommands();
  const { push: pushNotification } = useNotificationCenter();
  const highlightTerm = searchTerm.trim();
  const renderHighlight = useCallback((value: string) => highlightMatches(value, highlightTerm), [highlightTerm]);

  const lastProjectPinned = useMemo(() => {
    if (!lastProject) return false;
    return recents.some((item) => item.path === lastProject.path && item.pinned);
  }, [lastProject, recents]);

  const sortedRecents = useMemo(() => {
    const term = searchTerm.trim().toLowerCase();
    const filtered = recents.filter((item) => {
      if (!term) return true;
      const haystack = [item.name, item.path, item.template ?? '', item.colorSpace ?? '']
        .filter(Boolean)
        .map((value) => value.toLowerCase());
      return haystack.some((value) => value.includes(term));
    });

    return filtered.sort(compareRecents);
  }, [recents, searchTerm]);

  const projectHistory = useMemo(() => {
    const ordered = [...recents].sort(compareRecents);
    return ordered.filter((item) => (lastProject ? item.path !== lastProject.path : true)).slice(0, 4);
  }, [lastProject, recents]);

  const delayOptions = useMemo(
    () => buildDelayOptions(SPLASH_AUTO_DELAY_PRESETS, autoDelay),
    [autoDelay],
  );

  const fallbackOptions = useMemo(
    () => buildDelayOptions(SPLASH_FALLBACK_DELAY_PRESETS, fallbackDelay),
    [fallbackDelay],
  );

  useEffect(() => {
    if (!sortedRecents.length) {
      setFocusedIndex(-1);
      return;
    }

    setFocusedIndex((prev) => {
      if (prev < 0) return 0;
      if (prev >= sortedRecents.length) return Math.max(0, sortedRecents.length - 1);
      const currentPath = sortedRecents[prev]?.path;
      const matchingIndex = sortedRecents.findIndex((item) => item.path === currentPath);
      return matchingIndex >= 0 ? matchingIndex : 0;
    });
  }, [sortedRecents]);

  const dispatchRecents = useCallback((action: RecentsAction) => {
    setRecents((prev) => {
      const next = recentsReducer(prev, action);
      if (next === prev) {
        return prev;
      }
      persistRecents(next);
      return next;
    });
  }, []);

  const updateRecents = useCallback(
    (entry: Partial<Recent> & { path: string }) => {
      dispatchRecents({ type: 'update', entry });
    },
    [dispatchRecents],
  );

  const fetchRecents = useCallback(async () => {
    setIsLoading(true);
    setLoadError(null);
    const stored = loadRecentsFromStorage();

    try {
      const engineRecents = await engine.listRecent();
      const merged = mergeRecentCollections(stored, [], engineRecents);
      dispatchRecents({ type: 'hydrate', payload: merged });
      setLoadError(null);
    } catch (error) {
      console.warn('[Hub] Failed to fetch recent projects', error);
      setLoadError(error instanceof Error ? error.message : 'Unable to load recent projects');
      dispatchRecents({ type: 'hydrate', payload: stored });
    } finally {
      setIsLoading(false);
    }
  }, [dispatchRecents]);

  useEffect(() => {
    fetchRecents();
  }, [fetchRecents]);

  useEffect(() => {
    const off = engine.on('recents:updated', (payload) => {
      if (!Array.isArray(payload)) return;
      dispatchRecents({ type: 'merge', collections: [payload as Recent[]] });
    });

    return () => {
      off?.();
    };
  }, [dispatchRecents]);

  useEffect(() => {
    const off = engine.on('lastProject:updated', (payload) => {
      if (!payload) {
        setLastProject(null);
        return;
      }
      setLastProject(payload as LastProject);
    });

    return () => {
      off?.();
    };
  }, []);

  useEffect(() => {
    if (typeof window === 'undefined') return;
    window.localStorage.setItem(SPLASH_AUTO_CONTINUE_KEY, autoContinue ? 'true' : 'false');
  }, [autoContinue]);

  useEffect(() => {
    writeAutoDelayPreference(autoDelay);
  }, [autoDelay]);

  useEffect(() => {
    writeFallbackDelayPreference(fallbackDelay);
  }, [fallbackDelay]);

  const openProjectByPath = useCallback(
    async (path: string) => {
      if (!path) return;
      const trimmed = path.trim();
      if (!trimmed) return;

      setBusyPath(trimmed);
      setActionError(null);

      try {
        await loadProject(trimmed);
        const name = deriveNameFromPath(trimmed);
        const timestamp = new Date().toISOString();
        updateRecents({
          name,
          path: trimmed,
          last: timestamp,
        });
        setLastProject({
          name,
          path: trimmed,
          lastOpened: timestamp,
          template: null,
        });
        nav('/editor');
      } catch (error) {
        console.error('[Hub] Failed to open project', error);
        const message = error instanceof Error ? error.message : 'Unable to open project';
        setActionError(message);
        pushNotification({ level: 'error', title: 'Open project failed', message, durationMs: 8000 });
      } finally {
        setBusyPath(null);
      }
    },
      [loadProject, nav, pushNotification, updateRecents],
  );

  const handleOpenFromDisk = useCallback(async () => {
    setActionError(null);
    try {
      const selected = await engine.browseForProject();
      if (!selected) return;
      await openProjectByPath(selected);
    } catch (error) {
      console.error('[Hub] File dialog failed', error);
      const message = error instanceof Error ? error.message : 'Unable to open file dialog';
      setActionError(message);
      pushNotification({ level: 'error', title: 'Open project failed', message, durationMs: 6000 });
    }
  }, [openProjectByPath, pushNotification]);

  const handleForgetLastProject = useCallback(() => {
    const stalePath = lastProject?.path ?? null;
    engine.clearLastProject();
    setLastProject(null);
    setActionError(null);
    if (stalePath) {
      dispatchRecents({ type: 'remove', path: stalePath });
    }
  }, [dispatchRecents, lastProject]);

  const handleToggleLastProjectPin = useCallback(() => {
    if (!lastProject) return;
    dispatchRecents({
      type: 'update',
      entry: {
        path: lastProject.path,
        name: lastProject.name,
        template: lastProject.template ?? null,
        last: lastProject.lastOpened,
        pinned: !lastProjectPinned,
      },
    });
  }, [dispatchRecents, lastProject, lastProjectPinned]);

  const handleDelayChange = useCallback((event: ChangeEvent<HTMLSelectElement>) => {
    const nextValue = Number(event.target.value);
    if (Number.isNaN(nextValue)) return;
    setAutoDelay(clampAutoDelay(nextValue));
  }, []);

  const handleFallbackDelayChange = useCallback((event: ChangeEvent<HTMLSelectElement>) => {
    const nextValue = Number(event.target.value);
    if (Number.isNaN(nextValue)) return;
    setFallbackDelay(clampFallbackDelay(nextValue));
  }, []);

  const handleFormChange = useCallback((patch: Partial<CreateFormState>) => {
    setForm((prev) => ({ ...prev, ...patch }));
  }, []);

  const handleBrowseLocation = useCallback(async () => {
    try {
      const selected = await engine.browseForFolder();
      if (!selected) return;
      handleFormChange({ location: selected });
      setCreateError(null);
    } catch (error) {
      console.error('[Hub] File dialog failed', error);
      const message = error instanceof Error ? error.message : 'Unable to open folder dialog';
      setCreateError(message);
      pushNotification({ level: 'error', title: 'Folder dialog failed', message, durationMs: 6000 });
    }
  }, [handleFormChange, pushNotification, setCreateError]);

  const handleRemoveRecent = useCallback(
    (path: string) => {
      dispatchRecents({ type: 'remove', path });
      setActiveMenuPath((current) => (current === path ? null : current));
    },
    [dispatchRecents],
  );

  const handleTogglePin = useCallback(
    (path: string) => {
      dispatchRecents({ type: 'togglePin', path });
      setActiveMenuPath(null);
    },
    [dispatchRecents],
  );

  const handleOpenRename = useCallback((project: Recent) => {
    setActiveMenuPath(null);
    setRenameTarget({ path: project.path, name: project.name });
    setRenameValue(project.name ?? deriveNameFromPath(project.path));
  }, []);

  const handleRenameSubmit = useCallback(
    (event?: FormEvent<HTMLFormElement>) => {
      event?.preventDefault();
      if (!renameTarget) return;

      const nextName = renameValue.trim();
      if (!nextName.length) {
        setRenameValue(renameTarget.name);
        return;
      }

      dispatchRecents({ type: 'update', entry: { path: renameTarget.path, name: nextName } });
      setRenameTarget(null);
      setRenameValue('');
    },
    [dispatchRecents, renameTarget, renameValue],
  );

  const handleRenameCancel = useCallback(() => {
    setRenameTarget(null);
    setRenameValue('');
  }, []);

  const handleListKeyDown = useCallback(
    (event: KeyboardEvent<HTMLDivElement>) => {
      if (!sortedRecents.length) return;

      if (event.key === 'ArrowDown') {
        event.preventDefault();
        setFocusedIndex((prev) => {
          if (prev < 0) return 0;
          return prev >= sortedRecents.length - 1 ? 0 : prev + 1;
        });
        return;
      }

      if (event.key === 'ArrowUp') {
        event.preventDefault();
        setFocusedIndex((prev) => {
          if (prev < 0) return sortedRecents.length - 1;
          return prev === 0 ? sortedRecents.length - 1 : prev - 1;
        });
        return;
      }

      const current = sortedRecents[focusedIndex];
      if (!current) {
        return;
      }

      if (event.key === 'Enter') {
        event.preventDefault();
        void openProjectByPath(current.path);
        return;
      }

      if (event.key === 'Delete' || event.key === 'Backspace') {
        event.preventDefault();
        handleRemoveRecent(current.path);
        setFocusedIndex((prev) => {
          const next = Math.min(prev, sortedRecents.length - 2);
          return Math.max(next, -1);
        });
        return;
      }

      if ((event.key === 'p' || event.key === 'P') && event.ctrlKey) {
        event.preventDefault();
        handleTogglePin(current.path);
      }
    },
    [focusedIndex, handleRemoveRecent, handleTogglePin, openProjectByPath, sortedRecents],
  );

  const handleCopyPath = useCallback((path: string) => {
    setActiveMenuPath(null);

    if (typeof navigator !== 'undefined' && navigator.clipboard && typeof navigator.clipboard.writeText === 'function') {
      navigator.clipboard.writeText(path).catch((error) => {
        console.warn('[Hub] Failed to copy path', error);
      });
    } else if (typeof window !== 'undefined') {
      window.prompt('Copy project path', path);
    }
  }, []);

  useEffect(() => {
    if (!activeMenuPath) return;

    const handlePointer = () => setActiveMenuPath(null);
    document.addEventListener('pointerdown', handlePointer);
    return () => document.removeEventListener('pointerdown', handlePointer);
  }, [activeMenuPath]);

  const openCreateModal = useCallback((template?: TemplateSpec) => {
    setSelectedTemplate(template ?? null);
    setForm({
      name: template?.name ?? '',
      location: '',
      width: String(template?.preset.width ?? 1920),
      height: String(template?.preset.height ?? 1080),
      fps: String(template?.preset.fps ?? 60),
      colorSpace: template?.preset.colorSpace ?? 'Rec.709',
    });
    setCreateError(null);
    setIsCreateOpen(true);
  }, []);

  useEffect(() => {
    if (!location.state || typeof location.state !== 'object') return;
    const action = (location.state as { action?: string }).action;
    if (action === 'new-project') {
      openCreateModal();
      nav(location.pathname, { replace: true, state: null });
    }
  }, [location, nav, openCreateModal]);

  const closeCreateModal = useCallback(() => {
    if (isCreating) return;
    setIsCreateOpen(false);
    setSelectedTemplate(null);
    setForm(DEFAULT_FORM);
    setCreateError(null);
  }, [isCreating]);

  const handleCreateSubmit = useCallback(
    async (event: FormEvent<HTMLFormElement>) => {
      event.preventDefault();
      setCreateError(null);

      if (!form.location.trim()) {
        setCreateError('Please specify where the project should be stored');
        return;
      }

      setIsCreating(true);
      try {
        const payload: CreateProjectPayload = {
          name: form.name.trim() || (selectedTemplate?.name ?? 'New Project'),
          location: form.location.trim(),
          width: Number.parseInt(form.width, 10) || 1920,
          height: Number.parseInt(form.height, 10) || 1080,
          fps: Number.parseInt(form.fps, 10) || 60,
          colorSpace: form.colorSpace,
          template: selectedTemplate?.name,
        };

        const project = await engine.createProject(payload);
        const path = project?.path || payload.location;
        const name = project?.name || payload.name;
        updateRecents({
          name,
          path,
          last: new Date().toISOString(),
          template: project?.template ?? payload.template ?? null,
          width: project?.settings?.width ?? payload.width,
          height: project?.settings?.height ?? payload.height,
          fps: project?.settings?.fps ?? payload.fps,
          colorSpace: project?.settings?.colorSpace ?? payload.colorSpace,
        });
        setLastProject({
          name,
          path,
          lastOpened: new Date().toISOString(),
          template: project?.template ?? payload.template ?? null,
        });
        await loadProject(path);
        setIsCreateOpen(false);
        nav('/editor');
      } catch (error) {
        console.error('[Hub] Failed to create project', error);
        const message = error instanceof Error ? error.message : 'Unable to create project';
        setCreateError(message);
        pushNotification({ level: 'error', title: 'Create project failed', message, durationMs: 8000 });
      } finally {
        setIsCreating(false);
      }
    },
      [form, loadProject, nav, pushNotification, selectedTemplate, updateRecents],
  );

  return (
    <div className="relative min-h-screen overflow-hidden bg-[#030714] text-white">
      <div className="pointer-events-none absolute inset-0 overflow-hidden">
        <div className="absolute -top-48 right-[-10%] h-[520px] w-[520px] rounded-full bg-sky-500/25 blur-[240px]" />
        <div className="absolute inset-0 bg-[radial-gradient(circle_at_center,_rgba(15,118,254,0.16),_transparent_70%)] opacity-70" />
        <div className="absolute -bottom-56 left-[-20%] h-[520px] w-[520px] rounded-full bg-indigo-600/25 blur-[260px]" />
      </div>

      <div className="relative z-10 mx-auto w-full max-w-6xl px-6 py-12 space-y-10">
        <header className="space-y-6">
          <div className="space-y-4">
            <p className="text-xs uppercase tracking-[0.55em] text-sky-200/70">Command hub</p>
            <h1 className="text-4xl font-semibold leading-tight text-white">Manage your Vortex sessions</h1>
            <p className="text-base text-gray-400 max-w-3xl">
              Launch scenes, open recent layouts, and grab resources without leaving this screen.
            </p>
          </div>
          <div className="flex flex-wrap gap-3">
            <button
              type="button"
              onClick={() => openCreateModal()}
              className={`${ACCENT_BUTTON} min-w-[160px] justify-center`}
            >
              <span>＋</span>
              <span>New project</span>
            </button>
            <button
              type="button"
              onClick={handleOpenFromDisk}
              className={`${SECONDARY_BUTTON} min-w-[150px] justify-center`}
            >
              <span>📂</span>
              <span>Open project</span>
            </button>
            <button
              type="button"
              onClick={fetchRecents}
              className={`${GHOST_BUTTON} min-w-[120px] justify-center`}
            >
              <span>⟳</span>
              <span>Refresh</span>
            </button>
            {lastProject && (
              <button
                type="button"
                onClick={() => openProjectByPath(lastProject.path)}
                disabled={busyPath === lastProject.path}
                className={`${SECONDARY_BUTTON} min-w-[180px] justify-center disabled:opacity-60`}
                title={lastProject.path}
              >
                {busyPath === lastProject.path ? 'Opening…' : `Continue ${lastProject.name}`}
              </button>
            )}
          </div>
        </header>

        {lastProject && (
          <section className="glass-panel relative overflow-hidden rounded-3xl border border-white/10 p-6">
            <div className="pointer-events-none absolute inset-0 bg-[radial-gradient(circle_at_top,_rgba(99,102,241,0.18),_transparent_70%)]" />
            <div className="relative flex flex-col gap-4 lg:flex-row lg:items-center lg:justify-between">
              <div className="flex items-start gap-4">
                <div className="flex h-14 w-14 items-center justify-center rounded-2xl bg-gradient-to-br from-sky-500/50 to-indigo-500/40 text-2xl">
                  🎬
                </div>
                <div className="min-w-0 space-y-2">
                  <p className="text-[11px] uppercase tracking-[0.4em] text-sky-200/70">Continue last project</p>
                  <div className="truncate text-2xl font-semibold text-white" title={lastProject.name}>
                    {lastProject.name}
                  </div>
                  <div className="flex flex-wrap items-center gap-2 text-xs text-gray-300" title={lastProject.path}>
                    <span className="truncate max-w-full text-gray-400">{lastProject.path}</span>
                    {lastProject.template && (
                      <span className="rounded-full bg-white/10 px-2 py-0.5 text-[11px] text-gray-100">{lastProject.template}</span>
                    )}
                    {lastProject.width && lastProject.height && (
                      <span className="rounded-full bg-white/10 px-2 py-0.5 text-[11px] text-gray-100">
                        {lastProject.width}×{lastProject.height}
                      </span>
                    )}
                  </div>
                  {lastProject.lastOpened && (
                    <div className="text-xs text-sky-200/90">Opened {formatLastUsed(lastProject.lastOpened)}</div>
                  )}
                </div>
              </div>
              <div className="flex flex-col gap-3 lg:items-end">
                <div className="flex flex-wrap justify-end gap-3">
                  <button
                    type="button"
                    onClick={() => openProjectByPath(lastProject.path)}
                    disabled={busyPath === lastProject.path}
                    className={`${ACCENT_BUTTON} min-w-[140px] justify-center disabled:opacity-60`}
                  >
                    {busyPath === lastProject.path ? 'Opening…' : 'Continue'}
                  </button>
                  <button
                    type="button"
                    onClick={handleForgetLastProject}
                    className={`${SUBTLE_BUTTON} text-[11px]`}
                    aria-label="Forget this project"
                  >
                    Forget
                  </button>
                  <button
                    type="button"
                    onClick={handleToggleLastProjectPin}
                    className={`${SUBTLE_BUTTON} text-[11px]`}
                    aria-pressed={lastProjectPinned}
                  >
                    {lastProjectPinned ? 'Unpin' : 'Pin to recents'}
                  </button>
                </div>
                <div className="flex flex-wrap gap-3 text-xs text-gray-300 lg:justify-end">
                  <label className="flex items-center gap-2 rounded-xl border border-white/10 bg-black/30 px-3 py-2">
                    <input
                      type="checkbox"
                      checked={autoContinue}
                      onChange={(event) => setAutoContinue(event.target.checked)}
                      className="h-3.5 w-3.5 rounded border-white/30 bg-black/60 text-sky-400 focus:ring-sky-400"
                    />
                    Auto-continue
                  </label>
                  <label className="flex items-center gap-2 rounded-xl border border-white/15 bg-black/40 px-3 py-2 text-gray-200">
                    <span>Delay</span>
                    <select
                      value={autoDelay}
                      onChange={handleDelayChange}
                      className="rounded-lg border border-white/15 bg-black/80 px-2 py-1 text-gray-100 focus:border-sky-500/60 focus:outline-none"
                    >
                      {delayOptions.map((option) => (
                        <option key={option.value} value={option.value}>
                          {option.label}
                        </option>
                      ))}
                    </select>
                  </label>
                  <label className="flex items-center gap-2 rounded-xl border border-white/15 bg-black/40 px-3 py-2 text-gray-200">
                    <span>Fallback</span>
                    <select
                      value={fallbackDelay}
                      onChange={handleFallbackDelayChange}
                      className="rounded-lg border border-white/15 bg-black/80 px-2 py-1 text-gray-100 focus:border-sky-500/60 focus:outline-none"
                    >
                      {fallbackOptions.map((option) => (
                        <option key={option.value} value={option.value}>
                          {option.label}
                        </option>
                      ))}
                    </select>
                  </label>
                </div>
              </div>
            </div>
          </section>
        )}

        {actionError && (
          <div className="rounded-3xl border border-red-500/30 bg-red-500/10 px-6 py-4 text-sm text-red-100 shadow-[0_15px_60px_rgba(0,0,0,0.45)]">
            <p>{actionError}</p>
            <div className="flex flex-wrap gap-3 text-xs">
              <button
                type="button"
                onClick={handleOpenFromDisk}
                className="rounded-xl border border-red-500/40 px-3 py-1 text-red-100 transition hover:border-red-400"
              >
                Choose another file
              </button>
              <button
                type="button"
                onClick={handleForgetLastProject}
                className="rounded-xl border border-red-500/30 px-3 py-1 text-red-100 transition hover:border-red-400"
              >
                Forget last project
              </button>
            </div>
          </div>
        )}

        <div className="grid gap-8 lg:grid-cols-[2fr,1fr]">
          <section className="space-y-5 rounded-3xl border border-white/10 bg-black/30 p-6 shadow-[0_25px_80px_rgba(2,6,23,0.75)]">
            <div className="flex flex-col gap-4 md:flex-row md:items-center md:justify-between">
              <div className="space-y-1">
                <p className="text-[11px] uppercase tracking-[0.35em] text-sky-200/70">Navigator</p>
                <h2 className="text-2xl font-semibold">Project history</h2>
                <p className="text-sm text-gray-400">Jump back into the flows you touched most recently.</p>
              </div>
              <div className="flex flex-col gap-2 md:items-end">
                <div className="relative w-full md:w-64">
                  <input
                    type="search"
                    value={searchTerm}
                    onChange={(event) => setSearchTerm(event.target.value)}
                    placeholder="Search projects"
                    className="w-full rounded-2xl border border-white/10 bg-black/40 px-4 py-2 text-sm text-gray-100 placeholder:text-gray-500 focus:border-sky-400/70 focus:outline-none"
                  />
                  {searchTerm && (
                    <button
                      type="button"
                      onClick={() => setSearchTerm('')}
                      className="absolute right-3 top-1/2 -translate-y-1/2 text-xs text-gray-500 transition hover:text-gray-200"
                    >
                      ✕
                    </button>
                  )}
                </div>
                <div className="text-xs text-gray-500">
                  {isLoading && sortedRecents.length === 0 && <span>Loading…</span>}
                  {isLoading && sortedRecents.length > 0 && <span>Refreshing…</span>}
                  {!isLoading && loadError && <span className="text-red-300">{loadError}</span>}
                  {!isLoading && !loadError && <span>{sortedRecents.length} tracked</span>}
                </div>
              </div>
            </div>

            <div
              ref={listRef}
              tabIndex={sortedRecents.length ? 0 : -1}
              onKeyDown={handleListKeyDown}
              role="listbox"
              aria-label="Recent projects"
              className="glass-panel relative rounded-3xl border border-white/10 p-2 focus:outline-none focus:ring-2 focus:ring-sky-500/40"
            >
              {isLoading && sortedRecents.length === 0 ? (
                <div className="divide-y divide-white/5">
                  {Array.from({ length: 3 }).map((_, idx) => (
                    <div key={idx} className="flex items-center gap-4 px-5 py-4 animate-pulse">
                      <div className="h-12 w-12 rounded-2xl bg-white/5" />
                      <div className="flex-1 space-y-2">
                        <div className="h-3 w-1/3 rounded bg-white/10" />
                        <div className="h-2.5 w-2/3 rounded bg-white/5" />
                      </div>
                    </div>
                  ))}
                </div>
              ) : sortedRecents.length === 0 ? (
                <div className="px-8 py-12 text-center text-sm text-gray-300">
                  <div className="text-5xl mb-4">🌀</div>
                  <p className="text-base text-gray-100 mb-2">Your projects will appear here</p>
                  <p className="mb-6 text-gray-400">Create a new scene or pull an existing layout from disk.</p>
                  <div className="flex justify-center gap-3">
                    <button
                      type="button"
                      onClick={() => openCreateModal()}
                      className="rounded-2xl border border-white/15 bg-sky-500/10 px-4 py-2 text-sm text-sky-100 transition hover:border-sky-400/60"
                    >
                      New project
                    </button>
                    <button
                      type="button"
                      onClick={handleOpenFromDisk}
                      className="rounded-2xl border border-white/10 bg-white/5 px-4 py-2 text-sm text-gray-100 transition hover:border-sky-400/60"
                    >
                      Open file…
                    </button>
                  </div>
                </div>
              ) : (
                <div className="grid gap-4 p-4 sm:grid-cols-2">
                  {sortedRecents.map((project, index) => {
                    const detailChips: string[] = [];
                    if (project.template) detailChips.push(project.template);
                    if (project.width && project.height) detailChips.push(`${project.width}×${project.height}`);
                    if (project.fps) detailChips.push(`${project.fps} fps`);
                    if (project.colorSpace) detailChips.push(project.colorSpace);
                    const isFocused = index === focusedIndex;
                    const isMenuOpen = activeMenuPath === project.path;

                    return (
                      <article
                        key={project.path}
                        role="option"
                        aria-selected={isFocused}
                        tabIndex={-1}
                        onClick={() => openProjectByPath(project.path)}
                        onMouseEnter={() => setFocusedIndex(index)}
                        onFocus={() => setFocusedIndex(index)}
                        className={`group relative overflow-hidden rounded-2xl border bg-gradient-to-br from-white/10 to-white/[0.03] p-4 transition cursor-pointer hover:border-sky-400/60 hover:shadow-[0_20px_50px_rgba(8,47,73,0.55)] ${
                          isFocused ? 'border-sky-500/70 ring-2 ring-sky-400/40' : 'border-white/10'
                        }`}
                      >
                        <div
                          className="pointer-events-none absolute inset-0 opacity-0 transition group-hover:opacity-100"
                          style={{ background: 'radial-gradient(circle at top, rgba(56,189,248,0.25), transparent 70%)' }}
                        />
                        <div className="relative flex items-start gap-3">
                          <div className="flex h-11 w-11 items-center justify-center rounded-xl bg-white/10 text-lg">
                            {project.template ? <span>{project.template.slice(0, 1)}</span> : <span>🎬</span>}
                          </div>
                          <div className="min-w-0 flex-1 space-y-1">
                            <div className="flex items-center gap-2">
                              <span className="font-medium text-gray-100 truncate" title={project.name ?? deriveNameFromPath(project.path)}>
                                {renderHighlight(project.name ?? deriveNameFromPath(project.path))}
                              </span>
                              {project.pinned && (
                                <span className="rounded-full bg-amber-500/20 px-2 py-0.5 text-[10px] text-amber-200" title="Pinned" aria-label="Pinned">
                                  ★
                                </span>
                              )}
                              {project.error && (
                                <span className="rounded bg-red-500/20 px-2 py-0.5 text-[10px] uppercase tracking-wide text-red-200" title={project.error}>
                                  {project.error.length > 18 ? 'Issue' : project.error}
                                </span>
                              )}
                            </div>
                            <div className="text-xs text-gray-400 truncate" title={project.path}>
                              {renderHighlight(project.path)}
                            </div>
                            {detailChips.length > 0 && (
                              <div className="text-xs text-gray-400 flex flex-wrap gap-2">
                                {detailChips.map((chip) => (
                                  <span key={chip} className="rounded-full bg-white/5 px-2 py-0.5">
                                    {chip}
                                  </span>
                                ))}
                              </div>
                            )}
                          </div>
                          <div className="flex flex-col items-end gap-2">
                            <button
                              type="button"
                              onClick={(event) => {
                                event.stopPropagation();
                                setActiveMenuPath((current) => (current === project.path ? null : project.path));
                              }}
                              className="rounded-full border border-transparent px-2 py-1 text-xs text-gray-400 hover:border-ui-border hover:text-gray-100"
                              aria-haspopup="menu"
                              aria-expanded={isMenuOpen}
                            >
                              ⋮
                            </button>
                            {busyPath === project.path ? (
                              <span className="inline-block h-4 w-4 animate-spin rounded-full border-2 border-blue-400/70 border-t-transparent" />
                            ) : (
                              <span className="text-gray-500 text-xs">↗</span>
                            )}
                          </div>
                        </div>
                        <div className="relative mt-3 flex flex-wrap items-center justify-between gap-3 text-xs text-gray-300">
                          <span>Last opened {formatLastUsed(project.last)}</span>
                          <div className="flex flex-wrap gap-2">
                            <button
                              type="button"
                              onClick={(event) => {
                                event.stopPropagation();
                                handleTogglePin(project.path);
                              }}
                              className="rounded border border-transparent px-2 py-1 transition hover:border-amber-300/60 hover:text-amber-200"
                              aria-label={project.pinned ? 'Unpin project' : 'Pin project'}
                            >
                              {project.pinned ? 'Unpin' : 'Pin'}
                            </button>
                            <button
                              type="button"
                              onClick={(event) => {
                                event.stopPropagation();
                                handleOpenRename(project);
                              }}
                              className="rounded border border-transparent px-2 py-1 transition hover:border-blue-400/60 hover:text-blue-200"
                              aria-label="Rename project"
                            >
                              Rename
                            </button>
                          </div>
                        </div>

                        {isMenuOpen && (
                          <div className="absolute right-4 top-12 z-20 w-48 rounded-2xl border border-white/10 bg-black/80 shadow-[0_20px_45px_rgba(2,6,23,0.85)]">
                            <button
                              type="button"
                              onClick={() => openProjectByPath(project.path)}
                              className="block w-full px-4 py-2 text-left text-sm text-gray-200 transition hover:bg-white/5"
                            >
                              Open
                            </button>
                            <button
                              type="button"
                              onClick={() => handleTogglePin(project.path)}
                              className="block w-full px-4 py-2 text-left text-sm text-gray-200 transition hover:bg-white/5"
                            >
                              {project.pinned ? 'Unpin' : 'Pin to top'}
                            </button>
                            <button
                              type="button"
                              onClick={() => handleOpenRename(project)}
                              className="block w-full px-4 py-2 text-left text-sm text-gray-200 transition hover:bg-white/5"
                            >
                              Rename…
                            </button>
                            <button
                              type="button"
                              onClick={() => handleCopyPath(project.path)}
                              className="block w-full px-4 py-2 text-left text-sm text-gray-200 transition hover:bg-white/5"
                            >
                              Copy path
                            </button>
                            <button
                              type="button"
                              onClick={() => handleRemoveRecent(project.path)}
                              className="block w-full px-4 py-2 text-left text-sm text-red-300 transition hover:bg-red-500/20"
                            >
                              Remove from list
                            </button>
                          </div>
                        )}
                      </article>
                    );
                  })}
                </div>
              )}
            </div>
          </section>

          <aside className="space-y-6">
            <section className="space-y-5 rounded-3xl border border-white/10 bg-black/20 p-6 shadow-[0_20px_60px_rgba(2,6,23,0.65)]">
              <div className="space-y-1">
                <p className="text-[11px] uppercase tracking-[0.35em] text-sky-200/70">Presets</p>
                <h2 className="text-xl font-semibold text-white">Quick start</h2>
                <p className="text-sm text-gray-400">Pick a tuned template, tweak settings, and launch.</p>
              </div>

              <div className="space-y-3">
                {TEMPLATES.map((template) => (
                  <button
                    key={template.name}
                    type="button"
                    onClick={() => openCreateModal(template)}
                    className="group flex w-full items-center gap-4 rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-left transition hover:border-sky-400/70 hover:bg-white/10"
                  >
                    <span className={`flex h-12 w-12 items-center justify-center rounded-2xl text-xl ${template.accent}`}>{template.icon}</span>
                    <span className="flex-1">
                      <span className="block text-sm font-medium text-gray-100">{template.name}</span>
                      <span className="block text-xs text-gray-500">{template.description}</span>
                    </span>
                    <span className="text-xs text-sky-300 transition group-hover:text-white">Configure</span>
                  </button>
                ))}
              </div>
            </section>

            <section className="space-y-5 rounded-3xl border border-white/10 bg-black/20 p-6 shadow-[0_20px_60px_rgba(2,6,23,0.65)]">
              <div className="space-y-1">
                <p className="text-[11px] uppercase tracking-[0.35em] text-purple-200/70">Guides</p>
                <h2 className="text-xl font-semibold text-white">Resources</h2>
                <p className="text-sm text-gray-400">Documentation, community, and planning tools.</p>
              </div>
              <div className="space-y-3 text-sm">
                <a
                  className="flex items-center justify-between rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-gray-200 transition hover:border-sky-400/70 hover:text-white"
                  href="https://github.com/RRotoko/Vortex"
                  target="_blank"
                  rel="noreferrer"
                >
                  <span>📚 Engine documentation</span>
                  <span className="text-xs text-gray-400">↗</span>
                </a>
                <a
                  className="flex items-center justify-between rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-gray-200 transition hover:border-sky-400/70 hover:text-white"
                  href="https://discord.gg/"
                  target="_blank"
                  rel="noreferrer"
                >
                  <span>💬 Community & support</span>
                  <span className="text-xs text-gray-400">↗</span>
                </a>
                <button
                  type="button"
                  onClick={() => window.open('https://trello.com/', '_blank')}
                  className="flex w-full items-center justify-between rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-left text-gray-200 transition hover:border-sky-400/70 hover:text-white"
                >
                  <span>📝 Roadmap</span>
                  <span className="text-xs text-gray-400">↗</span>
                </button>
              </div>
            </section>
          </aside>
        </div>
      </div>

      <CreateProjectModal
        isOpen={isCreateOpen}
        onClose={closeCreateModal}
        form={form}
        onChange={handleFormChange}
        onSubmit={handleCreateSubmit}
        onBrowseLocation={handleBrowseLocation}
        selectedTemplate={selectedTemplate}
        error={createError}
        isSubmitting={isCreating}
      />

      {renameTarget && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-sm p-4">
          <form
            className="w-full max-w-md space-y-4 rounded-2xl border border-ui-border bg-ui-panel p-6"
            onSubmit={handleRenameSubmit}
          >
            <div>
              <h3 className="text-lg font-semibold text-gray-100">Rename project</h3>
              <p className="text-sm text-gray-500">Update how "{renameTarget.name}" appears in recents.</p>
            </div>
            <label className="block space-y-1 text-sm text-gray-300">
              <span>Display name</span>
              <input
                autoFocus
                value={renameValue}
                onChange={(event) => setRenameValue(event.target.value)}
                className="w-full rounded-lg border border-ui-border bg-black/30 px-3 py-2 text-gray-100 focus:border-blue-500/60 focus:outline-none"
                placeholder="Project name"
              />
            </label>
            <div className="flex justify-end gap-3 text-sm">
              <button
                type="button"
                onClick={handleRenameCancel}
                className="rounded-lg border border-transparent px-4 py-2 text-gray-400 transition hover:border-ui-border"
              >
                Cancel
              </button>
              <button
                type="submit"
                className="rounded-lg border border-blue-500/50 bg-blue-500/20 px-4 py-2 text-blue-100 transition hover:bg-blue-500/30"
              >
                Save
              </button>
            </div>
          </form>
        </div>
      )}
    </div>
  );
}
