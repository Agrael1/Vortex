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
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 px-4 backdrop-blur-sm" onClick={handleBackdropClick}>
      <div
        className="w-full max-w-4xl overflow-hidden rounded-2xl border border-ui-border/70 bg-ui-panel shadow-2xl"
        onClick={stopPropagation}
      >
        <header className="flex items-start justify-between gap-4 border-b border-ui-border/70 p-6">
          <div className="space-y-2">
            <h3 className="text-2xl font-semibold text-white">Create New Project</h3>
            <p className="text-sm text-gray-400">
              Configure the initial scene parameters. You can tweak everything later inside the editor.
            </p>
          </div>
          <button
            type="button"
            onClick={onClose}
            className="rounded-lg px-3 py-1.5 text-sm text-gray-400 transition hover:bg-white/5 hover:text-white"
            disabled={isSubmitting}
          >
            ✕
          </button>
        </header>

        <div className="flex flex-col gap-6 p-6 md:flex-row">
          <aside className="md:w-64">
            <div className="rounded-xl border border-white/5 bg-black/20 p-4">
              <span className="block text-xs uppercase tracking-[0.3em] text-gray-500">Template</span>
              {selectedTemplate ? (
                <div className="mt-3 space-y-2 text-sm text-gray-200">
                  <div className="flex items-center gap-3">
                    <span className="text-lg">{selectedTemplate.icon}</span>
                    <div>
                      <div className="font-medium text-gray-100">{selectedTemplate.name}</div>
                      <div className="text-xs text-gray-400">{selectedTemplate.description}</div>
                    </div>
                  </div>
                  {templateSummary && (
                    <div className="rounded-lg border border-white/10 bg-white/5 px-3 py-2 text-xs text-gray-300">{templateSummary}</div>
                  )}
                  <p className="text-xs text-gray-500">
                    Want a different preset? Close this dialog and pick another option from Quick start.
                  </p>
                </div>
              ) : (
                <p className="mt-3 text-sm text-gray-400">No template selected. The default Rec.709 1080p preset will be used.</p>
              )}
            </div>
          </aside>

          <form className="flex-1 space-y-5" onSubmit={onSubmit}>
            <div className="grid gap-4 md:grid-cols-2">
              <label className="space-y-2 text-sm">
                <span className="text-gray-300">Name</span>
                <input
                  ref={nameInputRef}
                  value={form.name}
                  onChange={(event) => onChange({ name: event.target.value })}
                  placeholder="e.g., Production Show"
                  className="w-full rounded-lg border border-ui-border bg-black/40 px-3 py-2 text-sm text-white outline-none focus:border-blue-500/70"
                />
              </label>
              <label className="space-y-2 text-sm">
                <span className="text-gray-300">Location</span>
                <div className="flex items-center gap-2">
                  <input
                    value={form.location}
                    onChange={(event) => onChange({ location: event.target.value })}
                    placeholder="C:\\Projects\\Show\\main.vortex"
                    className="w-full rounded-lg border border-ui-border bg-black/40 px-3 py-2 text-sm text-white outline-none focus:border-blue-500/70"
                  />
                  <button
                    type="button"
                    onClick={onBrowseLocation}
                    className="rounded-lg border border-ui-border bg-black/30 px-3 py-2 text-sm text-gray-300 transition hover:border-blue-500/60 hover:text-blue-300"
                    disabled={isSubmitting}
                  >
                    Browse…
                  </button>
                </div>
              </label>
            </div>

            <div className="grid gap-4 md:grid-cols-3">
              <label className="space-y-2 text-sm">
                <span className="text-gray-300">Width</span>
                <input
                  value={form.width}
                  onChange={(event) => onChange({ width: event.target.value })}
                  className="w-full rounded-lg border border-ui-border bg-black/40 px-3 py-2 text-sm text-white outline-none focus:border-blue-500/70"
                />
              </label>
              <label className="space-y-2 text-sm">
                <span className="text-gray-300">Height</span>
                <input
                  value={form.height}
                  onChange={(event) => onChange({ height: event.target.value })}
                  className="w-full rounded-lg border border-ui-border bg-black/40 px-3 py-2 text-sm text-white outline-none focus:border-blue-500/70"
                />
              </label>
              <label className="space-y-2 text-sm">
                <span className="text-gray-300">FPS</span>
                <input
                  value={form.fps}
                  onChange={(event) => onChange({ fps: event.target.value })}
                  className="w-full rounded-lg border border-ui-border bg-black/40 px-3 py-2 text-sm text-white outline-none focus:border-blue-500/70"
                />
              </label>
            </div>

            <label className="space-y-2 text-sm">
              <span className="text-gray-300">Color space</span>
              <select
                value={form.colorSpace}
                onChange={(event) => onChange({ colorSpace: event.target.value })}
                className="w-full rounded-lg border border-ui-border bg-black/40 px-3 py-2 text-sm text-white outline-none focus:border-blue-500/70"
              >
                <option value="Rec.709">Rec.709</option>
                <option value="Rec.2020">Rec.2020</option>
                <option value="sRGB">sRGB</option>
              </select>
            </label>

            {error && <div className="rounded-lg border border-red-500/40 bg-red-900/20 px-3 py-2 text-sm text-red-200">{error}</div>}

            <div className="flex items-center justify-between gap-4 rounded-lg border border-white/5 bg-white/5 px-4 py-3 text-sm text-gray-400">
              <div>Project files will be generated at the selected location. You can adjust all parameters later in the editor.</div>
            </div>

            <div className="flex justify-end gap-3">
              <button
                type="button"
                onClick={onClose}
                className="rounded-lg border border-ui-border bg-black/30 px-4 py-2 text-sm text-gray-300 transition hover:border-gray-500/80 hover:text-white"
                disabled={isSubmitting}
              >
                Cancel
              </button>
              <button
                type="submit"
                className="inline-flex items-center gap-2 rounded-lg border border-blue-500/60 bg-blue-600/80 px-4 py-2 text-sm font-medium text-white transition hover:bg-blue-500 disabled:cursor-not-allowed disabled:opacity-60"
                disabled={isSubmitting}
              >
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
