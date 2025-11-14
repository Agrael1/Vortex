import { FormEvent, useCallback, useEffect, useMemo, useState } from 'react'
import { useLocation, useNavigate } from 'react-router-dom'
import { CreateProjectModal } from '@/app/components/CreateProjectModal'
import { engine, type CreateProjectPayload, type Recent, type LastProject } from '@/app/services/ipc/cefBridge'

export type TemplateSpec = {
  name: string
  description: string
  icon: string
  accent: string
  preset: { width: number; height: number; fps: number; colorSpace: string }
}

export type CreateFormState = {
  name: string
  location: string
  width: string
  height: string
  fps: string
  colorSpace: string
}

const RECENTS_STORAGE_KEY = 'vortex.hub.recents'
const MAX_RECENTS = 30

const compareRecents = (a: Recent, b: Recent) => {
  const pinDiff = Number(Boolean(b.pinned)) - Number(Boolean(a.pinned))
  if (pinDiff !== 0) return pinDiff

  const timeA = Date.parse(a.last)
  const timeB = Date.parse(b.last)
  if (!Number.isNaN(timeA) && !Number.isNaN(timeB) && timeA !== timeB) {
    return timeB - timeA
  }

  return a.name.localeCompare(b.name)
}

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
]

const DEFAULT_FORM: CreateFormState = {
  name: '',
  location: '',
  width: '1920',
  height: '1080',
  fps: '60',
  colorSpace: 'Rec.709',
}

const deriveNameFromPath = (path: string) => {
  if (!path) return 'Untitled'
  const normalized = path.replace(/\\/g, '/').split('/')
  const last = normalized[normalized.length - 1] || path
  return last.replace(/\.[^.]+$/, '') || last
}

const formatLastUsed = (value: string) => {
  const ts = Date.parse(value)
  if (Number.isNaN(ts)) return 'Unknown date'

  const diff = Date.now() - ts
  const minutes = Math.round(diff / 60000)
  if (minutes <= 1) return 'just now'
  if (minutes < 60) return `${minutes} min ago`

  const hours = Math.round(minutes / 60)
  if (hours < 24) return `${hours} h ago`

  const days = Math.round(hours / 24)
  if (days < 7) return `${days} d ago`

  return new Date(ts).toLocaleDateString()
}

const loadRecentsFromStorage = (): Recent[] => {
  if (typeof window === 'undefined' || !('localStorage' in window)) return []
  try {
    const raw = localStorage.getItem(RECENTS_STORAGE_KEY)
    if (!raw) return []
    const parsed = JSON.parse(raw)
    if (!Array.isArray(parsed)) return []
    const normalized = parsed
      .map((item) => {
        if (!item || typeof item !== 'object') return null
        const rawPath = (item as any).path
        if (typeof rawPath !== 'string' || !rawPath.length) return null
        const nameValue = (item as any).name
        const lastValue = (item as any).last
        const name = typeof nameValue === 'string' && nameValue.trim().length ? nameValue.trim() : deriveNameFromPath(rawPath)
        const last = typeof lastValue === 'string' && lastValue.trim().length ? lastValue : new Date().toISOString()
        const template = typeof (item as any).template === 'string' ? (item as any).template : null
        const width = Number.isFinite((item as any).width) ? Number((item as any).width) : undefined
        const height = Number.isFinite((item as any).height) ? Number((item as any).height) : undefined
        const fps = Number.isFinite((item as any).fps) ? Number((item as any).fps) : undefined
        const colorSpace = typeof (item as any).colorSpace === 'string' ? (item as any).colorSpace : undefined
        const preview = typeof (item as any).preview === 'string' ? (item as any).preview : null
        const pinned = (item as any).pinned === true
        const error = typeof (item as any).error === 'string' ? (item as any).error : null

        const result: Recent = {
          name,
          path: rawPath,
          last,
          template,
          width,
          height,
          fps,
          colorSpace,
          preview,
          error,
        }

        if (pinned) {
          result.pinned = true
        }

        return result
      })
      .filter((item): item is Recent => Boolean(item))

    return normalized.sort(compareRecents)
  } catch {
    return []
  }
}

const persistRecents = (recents: Recent[]) => {
  if (typeof window === 'undefined' || !('localStorage' in window)) return
  try {
    const ordered = [...recents].sort(compareRecents)
    const limited = ordered.slice(0, MAX_RECENTS)
    localStorage.setItem(RECENTS_STORAGE_KEY, JSON.stringify(limited))
  } catch {
    /* ignore storage write errors */
  }
}

export function Hub() {
  const nav = useNavigate()
  const location = useLocation()
  const [recents, setRecents] = useState<Recent[]>(() => loadRecentsFromStorage())
  const [isLoading, setIsLoading] = useState(true)
  const [loadError, setLoadError] = useState<string | null>(null)
  const [busyPath, setBusyPath] = useState<string | null>(null)
  const [actionError, setActionError] = useState<string | null>(null)
  const [searchTerm, setSearchTerm] = useState('')
  const [activeMenuPath, setActiveMenuPath] = useState<string | null>(null)
  const [lastProject, setLastProject] = useState<LastProject | null>(() => engine.getLastProject())

  const [isCreateOpen, setIsCreateOpen] = useState(false)
  const [isCreating, setIsCreating] = useState(false)
  const [createError, setCreateError] = useState<string | null>(null)
  const [selectedTemplate, setSelectedTemplate] = useState<TemplateSpec | null>(null)
  const [form, setForm] = useState<CreateFormState>(DEFAULT_FORM)

  const sortedRecents = useMemo(() => {
    const term = searchTerm.trim().toLowerCase()
    const filtered = recents.filter((item) => {
      if (!term) return true
      const haystack = [item.name, item.path, item.template ?? '', item.colorSpace ?? '']
        .filter(Boolean)
        .map((value) => value.toLowerCase())
      return haystack.some((value) => value.includes(term))
    })

    return filtered.sort(compareRecents)
  }, [recents, searchTerm])

  const updateRecents = useCallback((entry: Partial<Recent> & { path: string }) => {
    setRecents((prev) => {
      const existing = prev.find((item) => item.path === entry.path)

      const next: Recent = {
        name: entry.name?.trim()?.length ? entry.name.trim() : existing?.name || deriveNameFromPath(entry.path),
        path: entry.path,
        last: entry.last ?? existing?.last ?? new Date().toISOString(),
        pinned: entry.pinned ?? existing?.pinned,
        template: entry.template ?? existing?.template ?? null,
        width: entry.width ?? existing?.width,
        height: entry.height ?? existing?.height,
        fps: entry.fps ?? existing?.fps,
        colorSpace: entry.colorSpace ?? existing?.colorSpace,
        preview: entry.preview ?? existing?.preview ?? null,
        error: entry.error ?? existing?.error ?? null,
      }

      const combined = [next, ...prev.filter((item) => item.path !== entry.path)]
      const ordered = combined.sort(compareRecents)
      const limited = ordered.slice(0, MAX_RECENTS)
      persistRecents(limited)
      return limited
    })
  }, [])

  const fetchRecents = useCallback(async () => {
    setIsLoading(true)
    setLoadError(null)

    try {
      const list = await engine.listRecent()
      if (list.length > 0) {
        const ordered = [...list].sort(compareRecents)
        setRecents(ordered)
        persistRecents(ordered)
      } else {
        setRecents(loadRecentsFromStorage())
      }
    } catch (error) {
      console.warn('[Hub] Failed to fetch recent projects', error)
      setLoadError(error instanceof Error ? error.message : 'Unable to load recent projects')
      setRecents(loadRecentsFromStorage())
    } finally {
      setIsLoading(false)
    }
  }, [])

  useEffect(() => {
    fetchRecents()
  }, [fetchRecents])

  useEffect(() => {
    const off = engine.on('lastProject:updated', (payload) => {
      if (!payload) {
        setLastProject(null)
        return
      }
      setLastProject(payload as LastProject)
    })

    return () => {
      off?.()
    }
  }, [])

  const openProjectByPath = useCallback(
    async (path: string) => {
      if (!path) return
      const trimmed = path.trim()
      if (!trimmed) return

      setBusyPath(trimmed)
      setActionError(null)

      try {
        const project = await engine.openProject(trimmed)
        const resolvedPath = project?.path || trimmed
        const name = project?.name || deriveNameFromPath(resolvedPath)
        updateRecents({
          name,
          path: resolvedPath,
          last: new Date().toISOString(),
          template: project?.template ?? null,
          width: project?.settings?.width,
          height: project?.settings?.height,
          fps: project?.settings?.fps,
          colorSpace: project?.settings?.colorSpace,
        })
        setLastProject({
          name,
          path: resolvedPath,
          lastOpened: new Date().toISOString(),
          template: project?.template ?? null,
        })
        nav('/editor')
      } catch (error) {
  console.error('[Hub] Failed to open project', error)
  setActionError(error instanceof Error ? error.message : 'Unable to open project')
      } finally {
        setBusyPath(null)
      }
    },
    [nav, updateRecents]
  )

  const handleOpenFromDisk = useCallback(async () => {
    try {
      const selected = await engine.browseForProject()
      if (!selected) return
      await openProjectByPath(selected)
    } catch (error) {
      console.error('[Hub] File dialog failed', error)
      setActionError(error instanceof Error ? error.message : 'Unable to open file dialog')
    }
  }, [openProjectByPath, setActionError])

  const handleFormChange = useCallback((patch: Partial<CreateFormState>) => {
    setForm((prev) => ({ ...prev, ...patch }))
  }, [])

  const handleBrowseLocation = useCallback(async () => {
    try {
      const selected = await engine.browseForFolder()
      if (!selected) return
      handleFormChange({ location: selected })
      setCreateError(null)
    } catch (error) {
      console.error('[Hub] File dialog failed', error)
      setCreateError(error instanceof Error ? error.message : 'Unable to open folder dialog')
    }
  }, [handleFormChange, setCreateError])

  const handleRemoveRecent = useCallback((path: string) => {
    setRecents((prev) => {
      const filtered = prev.filter((item) => item.path !== path)
      const ordered = filtered.sort(compareRecents)
      persistRecents(ordered)
      return ordered
    })
    setActiveMenuPath((current) => (current === path ? null : current))
  }, [])

  const handleTogglePin = useCallback((path: string) => {
    setRecents((prev) => {
      const updated = prev.map((item) => (item.path === path ? { ...item, pinned: !item.pinned } : item))
      const ordered = updated.sort(compareRecents)
      persistRecents(ordered)
      return ordered
    })
    setActiveMenuPath(null)
  }, [])

  const handleCopyPath = useCallback((path: string) => {
    setActiveMenuPath(null)

    if (typeof navigator !== 'undefined' && navigator.clipboard && typeof navigator.clipboard.writeText === 'function') {
      navigator.clipboard.writeText(path).catch((error) => {
        console.warn('[Hub] Failed to copy path', error)
      })
    } else if (typeof window !== 'undefined') {
      window.prompt('Copy project path', path)
    }
  }, [])

  useEffect(() => {
    if (!activeMenuPath) return

    const handlePointer = () => setActiveMenuPath(null)
    document.addEventListener('pointerdown', handlePointer)
    return () => document.removeEventListener('pointerdown', handlePointer)
  }, [activeMenuPath])

  const openCreateModal = useCallback((template?: TemplateSpec) => {
    setSelectedTemplate(template ?? null)
    setForm({
      name: template?.name ?? '',
      location: '',
      width: String(template?.preset.width ?? 1920),
      height: String(template?.preset.height ?? 1080),
      fps: String(template?.preset.fps ?? 60),
      colorSpace: template?.preset.colorSpace ?? 'Rec.709',
    })
    setCreateError(null)
    setIsCreateOpen(true)
  }, [])

  useEffect(() => {
    if (!location.state || typeof location.state !== 'object') return
    const action = (location.state as { action?: string }).action
    if (action === 'new-project') {
      openCreateModal()
      nav(location.pathname, { replace: true, state: null })
    }
  }, [location, nav, openCreateModal])

  const closeCreateModal = useCallback(() => {
    if (isCreating) return
    setIsCreateOpen(false)
    setSelectedTemplate(null)
    setForm(DEFAULT_FORM)
    setCreateError(null)
  }, [isCreating])

  const handleCreateSubmit = useCallback(
    async (event: FormEvent<HTMLFormElement>) => {
      event.preventDefault()
      setCreateError(null)

      if (!form.location.trim()) {
        setCreateError('Please specify where the project should be stored')
        return
      }

      setIsCreating(true)
      try {
        const payload: CreateProjectPayload = {
          name: form.name.trim() || (selectedTemplate?.name ?? 'New Project'),
          location: form.location.trim(),
          width: Number.parseInt(form.width, 10) || 1920,
          height: Number.parseInt(form.height, 10) || 1080,
          fps: Number.parseInt(form.fps, 10) || 60,
          colorSpace: form.colorSpace,
          template: selectedTemplate?.name,
        }

        const project = await engine.createProject(payload)
        const path = project?.path || payload.location
        const name = project?.name || payload.name
        updateRecents({
          name,
          path,
          last: new Date().toISOString(),
          template: project?.template ?? payload.template ?? null,
          width: project?.settings?.width ?? payload.width,
          height: project?.settings?.height ?? payload.height,
          fps: project?.settings?.fps ?? payload.fps,
          colorSpace: project?.settings?.colorSpace ?? payload.colorSpace,
        })
        setLastProject({
          name,
          path,
          lastOpened: new Date().toISOString(),
          template: project?.template ?? payload.template ?? null,
        })
        setIsCreateOpen(false)
        nav('/editor')
      } catch (error) {
        console.error('[Hub] Failed to create project', error)
        setCreateError(error instanceof Error ? error.message : 'Unable to create project')
      } finally {
        setIsCreating(false)
      }
    },
    [form, nav, selectedTemplate, updateRecents]
  )

  return (
    <div className="min-h-screen bg-ui-bg text-white">
      <div className="max-w-6xl mx-auto w-full px-6 py-12 space-y-10">
        <header className="flex flex-col gap-6 md:flex-row md:items-center md:justify-between">
          <div className="space-y-2">
            <span className="text-xs uppercase tracking-[0.4em] text-gray-500">Vortex Hub</span>
            <h1 className="text-3xl font-semibold">Welcome back</h1>
            <p className="text-sm text-gray-400 max-w-xl">
              Manage projects, templates, and reference material from a single control center.
            </p>
          </div>
          <div className="flex flex-wrap gap-3">
            {lastProject && (
              <button
                type="button"
                onClick={() => openProjectByPath(lastProject.path)}
                disabled={busyPath === lastProject.path}
                className="inline-flex items-center gap-2 rounded-lg border border-transparent bg-white/10 px-4 py-2 text-sm font-medium text-gray-200 transition hover:border-blue-500/60 hover:text-blue-200 disabled:opacity-60"
                title={lastProject.path}
              >
                <span>←</span>
                <span className="truncate max-w-[160px]">Back to {lastProject.name}</span>
              </button>
            )}
            <button
              type="button"
              onClick={() => openCreateModal()}
              className="inline-flex items-center gap-2 rounded-lg border border-ui-border bg-ui-panel px-4 py-2 text-sm font-medium transition hover:border-blue-500/60 hover:text-blue-300"
            >
              <span>＋</span>
              <span>New project</span>
            </button>
            <button
              type="button"
              onClick={handleOpenFromDisk}
              className="inline-flex items-center gap-2 rounded-lg border border-ui-border bg-black/30 px-4 py-2 text-sm font-medium transition hover:border-blue-500/60 hover:text-blue-300"
            >
              <span>📂</span>
              <span>Open…</span>
            </button>
            <button
              type="button"
              onClick={fetchRecents}
              className="inline-flex items-center gap-2 rounded-lg border border-transparent bg-white/5 px-4 py-2 text-sm font-medium text-gray-300 transition hover:bg-white/10"
            >
              <span>⟳</span>
              <span>Refresh</span>
            </button>
          </div>
        </header>

        {actionError && (
          <div className="rounded-lg border border-red-500/40 bg-red-900/20 px-4 py-3 text-sm text-red-200">
            {actionError}
          </div>
        )}

        <div className="grid gap-8 lg:grid-cols-[2fr,1fr]">
          <section className="space-y-4">
            <div className="flex flex-col gap-3 md:flex-row md:items-center md:justify-between">
              <div>
                <h2 className="text-xl font-semibold">Recent projects</h2>
                <p className="text-sm text-gray-500">Your latest scenes and workspaces</p>
              </div>
              <div className="flex flex-col gap-2 md:flex-row md:items-center md:gap-4">
                <div className="relative">
                  <input
                    type="search"
                    value={searchTerm}
                    onChange={(event) => setSearchTerm(event.target.value)}
                    placeholder="Search projects…"
                    className="w-full rounded-lg border border-ui-border bg-black/40 px-3 py-1.5 text-sm text-gray-200 placeholder:text-gray-500 focus:border-blue-500/60 focus:outline-none md:w-64"
                  />
                  {searchTerm && (
                    <button
                      type="button"
                      onClick={() => setSearchTerm('')}
                      className="absolute right-2 top-1/2 -translate-y-1/2 text-xs text-gray-500 hover:text-gray-200"
                    >
                      ✕
                    </button>
                  )}
                </div>
                <div className="text-xs text-gray-500 text-right">
                  {isLoading && <span>Loading…</span>}
                  {!isLoading && loadError && <span className="text-red-300">{loadError}</span>}
                  {!isLoading && !loadError && <span>{sortedRecents.length} items</span>}
                </div>
              </div>
            </div>

            <div className="overflow-hidden rounded-2xl border border-ui-border bg-ui-panel">
              {isLoading ? (
                <div className="divide-y divide-ui-border/70">
                  {Array.from({ length: 3 }).map((_, idx) => (
                    <div key={idx} className="flex items-center gap-4 px-5 py-4 animate-pulse">
                      <div className="h-12 w-12 rounded-lg bg-slate-700/50" />
                      <div className="flex-1 space-y-2">
                        <div className="h-3 w-1/3 rounded bg-slate-700/60" />
                        <div className="h-2.5 w-2/3 rounded bg-slate-700/40" />
                      </div>
                    </div>
                  ))}
                </div>
              ) : sortedRecents.length === 0 ? (
                <div className="px-8 py-12 text-center text-sm text-gray-400">
                  <div className="text-4xl mb-4">🌀</div>
                  <p className="text-base text-gray-200 mb-2">Your projects will appear here</p>
                  <p className="mb-4">Create a new scene or provide a path to an existing project file.</p>
                  <div className="flex justify-center gap-3">
                    <button
                      type="button"
                      onClick={() => openCreateModal()}
                      className="rounded-lg border border-ui-border bg-white/5 px-4 py-2 text-sm text-gray-200 transition hover:border-blue-500/60 hover:text-blue-300"
                    >
                      New project
                    </button>
                    <button
                      type="button"
                      onClick={handleOpenFromDisk}
                      className="rounded-lg border border-ui-border bg-black/30 px-4 py-2 text-sm text-gray-200 transition hover:border-blue-500/60 hover:text-blue-300"
                    >
                      Open file…
                    </button>
                  </div>
                </div>
              ) : (
                <div className="divide-y divide-ui-border/70">
                  {sortedRecents.map((project) => {
                    const isMenuOpen = activeMenuPath === project.path
                    const details = []
                    if (project.template) details.push(project.template)
                    if (project.width && project.height) details.push(`${project.width}×${project.height}`)
                    if (project.fps) details.push(`${project.fps} fps`)
                    if (project.colorSpace) details.push(project.colorSpace)

                    return (
                      <div key={project.path} className="relative">
                        <button
                          type="button"
                          onClick={() => openProjectByPath(project.path)}
                          className="flex w-full items-center gap-4 px-5 py-4 text-left transition hover:bg-white/5"
                        >
                          <div className="flex h-12 w-12 items-center justify-center rounded-lg bg-white/5 text-lg">
                            {project.template ? (
                              <span>{project.template.slice(0, 1)}</span>
                            ) : (
                              <span>🎬</span>
                            )}
                          </div>
                          <div className="flex-1 min-w-0">
                            <div className="flex items-center gap-2">
                              <span className="font-medium text-gray-100 truncate" title={project.name}>
                                {project.name}
                              </span>
                              {project.pinned && <span className="text-xs text-amber-300">★</span>}
                              {project.error && <span className="rounded bg-red-500/20 px-2 py-0.5 text-[10px] uppercase tracking-wide text-red-200">Issue</span>}
                            </div>
                            <div className="mt-1 text-xs text-gray-500 break-all" title={project.path}>
                              {project.path}
                            </div>
                            {details.length > 0 && (
                              <div className="mt-1 text-xs text-gray-400 flex flex-wrap gap-2">
                                {details.map((pill) => (
                                  <span key={pill} className="rounded-full bg-white/5 px-2 py-0.5">
                                    {pill}
                                  </span>
                                ))}
                              </div>
                            )}
                          </div>
                          <div className="flex items-center gap-2">
                            <span className="text-xs text-gray-500">{formatLastUsed(project.last)}</span>
                            <button
                              type="button"
                              onClick={(event) => {
                                event.stopPropagation()
                                setActiveMenuPath((current) => (current === project.path ? null : project.path))
                              }}
                              className="rounded border border-transparent px-2 py-1 text-xs text-gray-400 hover:border-ui-border hover:text-gray-200"
                            >
                              ⋮
                            </button>
                            {busyPath === project.path ? (
                              <span className="inline-block h-4 w-4 animate-spin rounded-full border-2 border-blue-400/70 border-t-transparent" />
                            ) : (
                              <span className="text-gray-500">↗</span>
                            )}
                          </div>
                        </button>

                        {isMenuOpen && (
                          <div className="absolute right-6 top-12 z-20 w-48 rounded-lg border border-ui-border bg-ui-panel shadow-lg">
                            <button
                              type="button"
                              onClick={() => openProjectByPath(project.path)}
                              className="block w-full px-4 py-2 text-left text-sm text-gray-200 hover:bg-white/5"
                            >
                              Open
                            </button>
                            <button
                              type="button"
                              onClick={() => handleTogglePin(project.path)}
                              className="block w-full px-4 py-2 text-left text-sm text-gray-200 hover:bg-white/5"
                            >
                              {project.pinned ? 'Unpin' : 'Pin to top'}
                            </button>
                            <button
                              type="button"
                              onClick={() => handleCopyPath(project.path)}
                              className="block w-full px-4 py-2 text-left text-sm text-gray-200 hover:bg-white/5"
                            >
                              Copy path
                            </button>
                            <button
                              type="button"
                              onClick={() => handleRemoveRecent(project.path)}
                              className="block w-full px-4 py-2 text-left text-sm text-red-300 hover:bg-red-500/20"
                            >
                              Remove from list
                            </button>
                          </div>
                        )}
                      </div>
                    )
                  })}
                </div>
              )}
            </div>
          </section>

          <aside className="space-y-6">
            <section className="space-y-4 rounded-2xl border border-ui-border bg-ui-panel p-6">
              <div>
                <h2 className="text-lg font-semibold">Quick start</h2>
                <p className="text-sm text-gray-500">Pick a preset and tailor the scene in a few clicks.</p>
              </div>

              <div className="space-y-3">
                {TEMPLATES.map((template) => (
                  <button
                    key={template.name}
                    type="button"
                    onClick={() => openCreateModal(template)}
                    className="flex w-full items-center gap-3 rounded-xl border border-transparent bg-white/5 px-4 py-3 text-left transition hover:border-blue-500/60 hover:text-blue-200"
                  >
                    <span className={`flex h-10 w-10 items-center justify-center rounded-lg ${template.accent}`}>{template.icon}</span>
                    <span className="flex-1">
                      <span className="block text-sm font-medium text-gray-100">{template.name}</span>
                      <span className="block text-xs text-gray-500">{template.description}</span>
                    </span>
                    <span className="text-xs text-blue-300">Configure</span>
                  </button>
                ))}
              </div>
            </section>

            <section className="space-y-4 rounded-2xl border border-ui-border bg-ui-panel p-6">
              <div>
                <h2 className="text-lg font-semibold">Resources</h2>
                <p className="text-sm text-gray-500">Documentation and support to keep moving fast.</p>
              </div>
              <div className="space-y-2 text-sm">
                <a
                  className="block rounded-lg border border-transparent px-3 py-2 text-gray-300 transition hover:border-blue-500/60 hover:bg-white/5 hover:text-blue-200"
                  href="https://github.com/RRotoko/Vortex"
                  target="_blank"
                  rel="noreferrer"
                >
                  📚 Engine documentation
                </a>
                <a
                  className="block rounded-lg border border-transparent px-3 py-2 text-gray-300 transition hover:border-blue-500/60 hover:bg-white/5 hover:text-blue-200"
                  href="https://discord.gg/"
                  target="_blank"
                  rel="noreferrer"
                >
                  💬 Community & support
                </a>
                <button
                  type="button"
                  onClick={() => window.open('https://trello.com/', '_blank')}
                  className="block w-full rounded-lg border border-transparent px-3 py-2 text-left text-gray-300 transition hover:border-blue-500/60 hover:bg-white/5 hover:text-blue-200"
                >
                  📝 Roadmap
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
    </div>
  )
}