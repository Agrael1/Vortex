import { FormEvent, MouseEventHandler, useEffect, useMemo, useRef } from 'react';
import { createPortal } from 'react-dom';
import type { CreateFormState, TemplateSpec } from '@/app/routes/Hub';

type Props = {
  isOpen: boolean;
  onClose: () => void;
  form: CreateFormState;
  onChange: (patch: Partial<CreateFormState>) => void;
  onSubmit: (event: FormEvent<HTMLFormElement>) => void;
  onBrowseLocation: () => void;
  selectedTemplate: TemplateSpec | null;
  error: string | null;
  isSubmitting: boolean;
};

const MODAL_PORTAL_TARGET: HTMLElement | null = typeof document !== 'undefined' ? document.body : null;
const FIELD_CLASS =
  'w-full rounded-2xl border border-white/15 bg-white/5 px-4 py-3 text-sm text-white placeholder:text-gray-500 focus:border-sky-400/70 focus:outline-none backdrop-blur-sm';
const SECTION_CARD = 'rounded-[24px] border border-white/10 bg-white/5 p-5 backdrop-blur-xl shadow-[0_25px_80px_rgba(2,6,23,0.65)]';
const GHOST_BUTTON =
  'rounded-2xl border border-white/10 bg-white/5 px-4 py-2 text-sm text-gray-200 transition hover:border-sky-400/60 hover:text-white';
const PRIMARY_BUTTON =
  'inline-flex items-center gap-2 rounded-2xl border border-sky-500/60 bg-gradient-to-r from-sky-500 to-indigo-500 px-5 py-2.5 text-sm font-medium text-white shadow-[0_20px_45px_rgba(15,118,254,0.4)] transition hover:from-sky-400 hover:to-indigo-400 disabled:opacity-60 disabled:cursor-not-allowed';

export function CreateProjectModal({
  isOpen,
  onClose,
  form,
  onChange,
  onSubmit,
  onBrowseLocation,
  selectedTemplate,
  error,
  isSubmitting,
}: Props) {
  const nameInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    if (!isOpen) return;

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        event.preventDefault();
        onClose();
      }
    };

    document.addEventListener('keydown', handleKeyDown);

    return () => document.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, onClose]);

  useEffect(() => {
    if (!isOpen) return;
    const previousOverflow = document.body.style.overflow;
    document.body.style.overflow = 'hidden';
    nameInputRef.current?.focus();

    return () => {
      document.body.style.overflow = previousOverflow;
    };
  }, [isOpen]);

  const templateSummary = useMemo(() => {
    if (!selectedTemplate) return null;
    const { preset } = selectedTemplate;
    return `${preset.width}×${preset.height} @ ${preset.fps}fps · ${preset.colorSpace}`;
  }, [selectedTemplate]);

  if (!isOpen || !MODAL_PORTAL_TARGET) return null;

  const handleBackdropClick = () => {
    if (!isSubmitting) {
      onClose();
    }
  };

  const stopPropagation: MouseEventHandler<HTMLDivElement> = (event) => {
    event.stopPropagation();
  };

  return createPortal(
    <div
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/80 px-4 py-8 backdrop-blur-[18px]"
      onClick={handleBackdropClick}
    >
      <div
        className="relative w-full max-w-5xl overflow-hidden rounded-[32px] border border-white/10 bg-[#030715]/90 shadow-[0_55px_150px_rgba(1,3,10,0.85)]"
        onClick={stopPropagation}
      >
        <div className="pointer-events-none absolute inset-0">
          <div className="absolute -top-32 right-[-10%] h-64 w-64 rounded-full bg-sky-500/30 blur-[140px]" />
          <div className="absolute -bottom-32 left-[-5%] h-64 w-64 rounded-full bg-indigo-500/20 blur-[140px]" />
        </div>

        <header className="relative flex items-start justify-between gap-6 border-b border-white/10 px-8 py-6">
          <div className="space-y-2">
            <p className="text-[11px] uppercase tracking-[0.35em] text-sky-200/70">Project setup</p>
            <h3 className="text-3xl font-semibold text-white">Create new project</h3>
            <p className="text-sm text-gray-400">
              Configure the launch preset. Every value stays editable later inside the editor.
            </p>
          </div>
          <button
            type="button"
            onClick={onClose}
            className="rounded-full border border-white/10 bg-white/5 px-3 py-1.5 text-sm text-gray-300 transition hover:border-sky-400/60 hover:text-white"
            disabled={isSubmitting}
          >
            ✕
          </button>
        </header>

        <div className="relative flex flex-col gap-6 p-8 md:flex-row">
          <aside className="md:w-72">
            <div className={`${SECTION_CARD} space-y-4`}>
              <span className="text-[11px] uppercase tracking-[0.35em] text-gray-500">Template</span>
              {selectedTemplate ? (
                <div className="space-y-3 text-sm text-gray-200">
                  <div className="flex items-center gap-3">
                    <span className="text-2xl">{selectedTemplate.icon}</span>
                    <div>
                      <div className="text-base font-semibold text-white">{selectedTemplate.name}</div>
                      <div className="text-xs text-gray-400">{selectedTemplate.description}</div>
                    </div>
                  </div>
                  {templateSummary && (
                    <div className="rounded-2xl border border-white/15 bg-black/30 px-3 py-2 text-xs text-gray-300">{templateSummary}</div>
                  )}
                  <p className="text-xs text-gray-500">
                    Need a different preset? Close this dialog and pick another card from Quick start.
                  </p>
                </div>
              ) : (
                <p className="text-sm text-gray-400">No template selected. The default Rec.709 1080p preset will be used.</p>
              )}
            </div>

            <div className="mt-5 space-y-2 text-xs text-gray-500">
              <p>Tip: you can drop a .vortex file anywhere in Hub to open it instantly.</p>
            </div>
          </aside>

          <form className="flex-1 space-y-5" onSubmit={onSubmit}>
            <div className="grid gap-5 md:grid-cols-2">
              <label className="space-y-1 text-sm text-gray-400">
                <span>Name</span>
                <input
                  ref={nameInputRef}
                  value={form.name}
                  onChange={(event) => onChange({ name: event.target.value })}
                  placeholder="e.g., Production Show"
                  className={FIELD_CLASS}
                />
              </label>
              <label className="space-y-1 text-sm text-gray-400">
                <span>Location</span>
                <div className="flex items-center gap-3">
                  <input
                    value={form.location}
                    onChange={(event) => onChange({ location: event.target.value })}
                    placeholder="C:\\Projects\\Show\\main.vortex"
                    className={FIELD_CLASS}
                  />
                  <button type="button" onClick={onBrowseLocation} className={`${GHOST_BUTTON} whitespace-nowrap`} disabled={isSubmitting}>
                    Browse
                  </button>
                </div>
              </label>
            </div>

            <div className="grid gap-4 md:grid-cols-3">
              <label className="space-y-1 text-sm text-gray-400">
                <span>Width</span>
                <input value={form.width} onChange={(event) => onChange({ width: event.target.value })} className={FIELD_CLASS} />
              </label>
              <label className="space-y-1 text-sm text-gray-400">
                <span>Height</span>
                <input value={form.height} onChange={(event) => onChange({ height: event.target.value })} className={FIELD_CLASS} />
              </label>
              <label className="space-y-1 text-sm text-gray-400">
                <span>FPS</span>
                <input value={form.fps} onChange={(event) => onChange({ fps: event.target.value })} className={FIELD_CLASS} />
              </label>
            </div>

            <label className="space-y-1 text-sm text-gray-400">
              <span>Color space</span>
              <select value={form.colorSpace} onChange={(event) => onChange({ colorSpace: event.target.value })} className={FIELD_CLASS}>
                <option value="Rec.709">Rec.709</option>
                <option value="Rec.2020">Rec.2020</option>
                <option value="sRGB">sRGB</option>
              </select>
            </label>

            {error && <div className="rounded-2xl border border-red-500/40 bg-red-500/10 px-4 py-3 text-sm text-red-200">{error}</div>}

            <div className={`${SECTION_CARD} flex items-center gap-3 text-sm text-gray-300`}>
              <div className="rounded-full bg-white/10 px-3 py-1 text-[11px] uppercase tracking-[0.2em] text-sky-200/80">Info</div>
              <p className="text-sm text-gray-300">
                Files are generated at the chosen location and synced automatically once you enter the editor.
              </p>
            </div>

            <div className="flex justify-end gap-3">
              <button type="button" onClick={onClose} className={GHOST_BUTTON} disabled={isSubmitting}>
                Cancel
              </button>
              <button type="submit" className={PRIMARY_BUTTON} disabled={isSubmitting}>
                {isSubmitting && <span className="h-4 w-4 animate-spin rounded-full border-2 border-white/80 border-t-transparent" />}
                <span>Create &amp; open</span>
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>,
    MODAL_PORTAL_TARGET,
  );
}
