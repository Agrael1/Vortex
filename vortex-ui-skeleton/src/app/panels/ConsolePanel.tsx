import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { useRecoilValue } from 'recoil';
import { consoleEntriesAtom } from '@state/atoms/editor';
import type { EngineLogEntry } from '@/app/services/ipc/cefBridge';
import { useConsoleActions } from '@state/hooks/useConsoleFeed';

const LEVELS: EngineLogEntry['level'][] = ['info', 'warn', 'error'];

const levelBadges: Record<EngineLogEntry['level'], string> = {
  info: 'border-slate-500/40 bg-slate-500/10 text-slate-200',
  warn: 'border-yellow-400/40 bg-yellow-500/10 text-yellow-200',
  error: 'border-red-500/40 bg-red-500/10 text-red-200',
};

export function ConsolePanel() {
  const entries = useRecoilValue(consoleEntriesAtom);
  const { clearLogs } = useConsoleActions();
  const [filters, setFilters] = useState<Record<EngineLogEntry['level'], boolean>>({ info: true, warn: true, error: true });
  const [autoScroll, setAutoScroll] = useState(true);
  const [query, setQuery] = useState('');
  const [isFilterVisible, setIsFilterVisible] = useState(false);
  const viewportRef = useRef<HTMLDivElement | null>(null);
  const filterInputRef = useRef<HTMLInputElement | null>(null);

  const normalizedQuery = query.trim().toLowerCase();
  const filteredEntries = useMemo(() => {
    return entries.filter((entry) => {
      const level = (entry.level ?? 'info') as EngineLogEntry['level'];
      if (!filters[level]) return false;
      if (!normalizedQuery.length) return true;
      const haystack = `${entry.message} ${entry.scope ?? ''}`.toLowerCase();
      return haystack.includes(normalizedQuery);
    });
  }, [entries, filters, normalizedQuery]);

  useEffect(() => {
    if (!autoScroll) return;
    const viewport = viewportRef.current;
    if (!viewport) return;
    viewport.scrollTo({ top: viewport.scrollHeight });
  }, [filteredEntries, autoScroll]);

  useEffect(() => {
    if (!isFilterVisible) return;
    const timer = window.setTimeout(() => filterInputRef.current?.focus(), 0);
    return () => window.clearTimeout(timer);
  }, [isFilterVisible]);

  const toggleLevel = useCallback((level: EngineLogEntry['level']) => {
    setFilters((prev) => ({ ...prev, [level]: !prev[level] }));
  }, []);

  const copyVisible = useCallback(() => {
    if (filteredEntries.length === 0) return;
    const payload = filteredEntries
      .map((entry) => {
        const timestamp = new Date(entry.time).toLocaleTimeString();
        const scope = entry.scope ? `[${entry.scope}] ` : '';
        const level = (entry.level ?? 'info').toUpperCase();
        return `[${timestamp}] ${level} ${scope}${entry.message}`;
      })
      .join('\n');

    if (navigator?.clipboard?.writeText) {
      navigator.clipboard.writeText(payload).catch(() => undefined);
    }
  }, [filteredEntries]);

  const handleManualScroll = useCallback(() => {
    const viewport = viewportRef.current;
    if (!viewport || !autoScroll) return;
    const nearBottom = viewport.scrollHeight - viewport.clientHeight - viewport.scrollTop < 40;
    if (!nearBottom) {
      setAutoScroll(false);
    }
  }, [autoScroll]);

  return (
    <div className="flex h-full w-full flex-col bg-[#050b16] text-xs font-mono text-white">
      <div className="flex flex-wrap items-center justify-between gap-3 border-b border-white/10 px-4 py-3">
        <div className="flex flex-wrap items-center gap-2">
          {LEVELS.map((level) => (
            <button
              key={level}
              type="button"
              onClick={() => toggleLevel(level)}
              className={`rounded-full border px-3 py-1 text-[11px] uppercase tracking-wide transition ${
                filters[level]
                  ? 'border-sky-400/60 bg-sky-500/10 text-sky-100'
                  : 'border-white/10 bg-white/5 text-gray-500 hover:border-sky-400/40 hover:text-sky-200'
              }`}
            >
              {level}
            </button>
          ))}
        </div>
        <div className="flex flex-wrap items-center gap-3 text-[11px] uppercase tracking-wide text-gray-400">
          <span>{filteredEntries.length} entries</span>
          <button
            type="button"
            onClick={() => setIsFilterVisible((prev) => !prev)}
            aria-pressed={isFilterVisible}
            className={`rounded-full border px-3 py-1 text-[11px] transition ${
              isFilterVisible
                ? 'border-sky-400/60 text-sky-100'
                : 'border-white/10 text-gray-500 hover:border-sky-400/40 hover:text-sky-200'
            }`}
          >
            {isFilterVisible ? 'Hide filter' : 'Show filter'}
            {query && !isFilterVisible && (
              <span className="ml-2 rounded-full bg-white/10 px-2 py-0.5 text-[10px] text-gray-200">
                {query.length > 8 ? `${query.slice(0, 8)}…` : query}
              </span>
            )}
          </button>
          <button type="button" onClick={copyVisible} className="text-cyan-300 hover:text-white">
            Copy
          </button>
          <button type="button" onClick={clearLogs} className="text-cyan-300 hover:text-white">
            Clear
          </button>
          <button
            type="button"
            onClick={() => setAutoScroll((prev) => !prev)}
            className={`rounded-full border px-3 py-1 text-[11px] ${autoScroll ? 'border-emerald-500/50 text-emerald-200' : 'border-white/10 text-gray-400'}`}
          >
            {autoScroll ? 'Autoscroll' : 'Manual'}
          </button>
        </div>
      </div>

      {isFilterVisible && (
        <div className="border-b border-white/10 px-4 py-2">
          <div className="relative">
            <input
              ref={filterInputRef}
              type="search"
              value={query}
              onChange={(event) => setQuery(event.target.value)}
              placeholder="Filter logs by text or scope"
              className="w-full rounded-lg border border-white/10 bg-black/50 px-3 py-2 pr-8 text-xs text-gray-200 placeholder:text-gray-500 focus:border-sky-400/60 focus:outline-none"
            />
            {query && (
              <button
                type="button"
                onClick={() => setQuery('')}
                className="absolute right-3 top-1/2 -translate-y-1/2 text-gray-500 transition hover:text-gray-200"
                aria-label="Clear filter"
              >
                ✕
              </button>
            )}
          </div>
        </div>
      )}

      <div
        ref={viewportRef}
        className="flex-1 overflow-y-auto overflow-x-hidden px-4 py-3 pr-2 space-y-2"
        onScroll={handleManualScroll}
        style={{ scrollbarWidth: 'thin' }}
      >
        {filteredEntries.map((entry) => {
          const level = (entry.level ?? 'info') as EngineLogEntry['level'];
          return (
            <article key={entry.id} className="rounded-2xl border border-white/5 bg-white/[0.03] px-3 py-2">
              <div className="flex flex-wrap items-center gap-2 text-[11px] text-gray-400">
                <span className="text-gray-500 w-20">{new Date(entry.time).toLocaleTimeString()}</span>
                <span className={`rounded-full border px-2 py-0.5 text-[10px] uppercase tracking-wide ${levelBadges[level]}`}>{level}</span>
                {entry.scope ? <span className="text-cyan-300">[{entry.scope}]</span> : null}
              </div>
              <pre className="mt-1 whitespace-pre-wrap break-words text-[12px] leading-relaxed text-slate-100">{entry.message}</pre>
            </article>
          );
        })}
        {filteredEntries.length === 0 ? (
          <div className="rounded-2xl border border-dashed border-white/10 px-4 py-6 text-center text-[11px] text-gray-500">
            No log entries match the current filters.
          </div>
        ) : null}
      </div>
    </div>
  );
}
