export const SPLASH_AUTO_CONTINUE_KEY = 'vortex.splash.autoContinue';
export const SPLASH_AUTO_DELAY_KEY = 'vortex.splash.autoDelay';
export const SPLASH_AUTO_DELAY_DEFAULT = 1200;
export const SPLASH_AUTO_DELAY_MIN = 500;
export const SPLASH_AUTO_DELAY_MAX = 5000;
export const SPLASH_FALLBACK_DELAY_KEY = 'vortex.splash.fallbackDelay';
export const SPLASH_FALLBACK_DELAY_DEFAULT = 3000;
export const SPLASH_FALLBACK_DELAY_MIN = 500;
export const SPLASH_FALLBACK_DELAY_MAX = 10000;
export const SPLASH_STICKY_KEY = 'vortex.splash.sticky';

export const clampAutoDelay = (value: number) => {
	if (!Number.isFinite(value)) return SPLASH_AUTO_DELAY_DEFAULT;
	if (value < SPLASH_AUTO_DELAY_MIN) return SPLASH_AUTO_DELAY_MIN;
	if (value > SPLASH_AUTO_DELAY_MAX) return SPLASH_AUTO_DELAY_MAX;
	return Math.round(value);
};

export const readAutoDelayPreference = () => {
	if (typeof window === 'undefined') return SPLASH_AUTO_DELAY_DEFAULT;
	const raw = window.localStorage.getItem(SPLASH_AUTO_DELAY_KEY);
	if (!raw) return SPLASH_AUTO_DELAY_DEFAULT;
	const parsed = Number.parseInt(raw, 10);
	if (Number.isNaN(parsed)) return SPLASH_AUTO_DELAY_DEFAULT;
	return clampAutoDelay(parsed);
};

export const writeAutoDelayPreference = (value: number) => {
	if (typeof window === 'undefined') return;
	const normalized = clampAutoDelay(value);
	window.localStorage.setItem(SPLASH_AUTO_DELAY_KEY, String(normalized));
};

export const clampFallbackDelay = (value: number) => {
	if (!Number.isFinite(value)) return SPLASH_FALLBACK_DELAY_DEFAULT;
	if (value < SPLASH_FALLBACK_DELAY_MIN) return SPLASH_FALLBACK_DELAY_MIN;
	if (value > SPLASH_FALLBACK_DELAY_MAX) return SPLASH_FALLBACK_DELAY_MAX;
	return Math.round(value);
};

export const readFallbackDelayPreference = () => {
	if (typeof window === 'undefined') return SPLASH_FALLBACK_DELAY_DEFAULT;
	const raw = window.localStorage.getItem(SPLASH_FALLBACK_DELAY_KEY);
	if (!raw) return SPLASH_FALLBACK_DELAY_DEFAULT;
	const parsed = Number.parseInt(raw, 10);
	if (Number.isNaN(parsed)) return SPLASH_FALLBACK_DELAY_DEFAULT;
	return clampFallbackDelay(parsed);
};

export const writeFallbackDelayPreference = (value: number) => {
	if (typeof window === 'undefined') return;
	const normalized = clampFallbackDelay(value);
	window.localStorage.setItem(SPLASH_FALLBACK_DELAY_KEY, String(normalized));
};

export const readSplashStickyPreference = () => {
	if (typeof window === 'undefined') return true;
	const raw = window.localStorage.getItem(SPLASH_STICKY_KEY);
	if (raw == null) {
		return true;
	}
	return raw === 'true';
};

export const writeSplashStickyPreference = (value: boolean) => {
	if (typeof window === 'undefined') return;
	window.localStorage.setItem(SPLASH_STICKY_KEY, value ? 'true' : 'false');
};

export const resolveFallbackDelayForContext = (baseDelay: number, hasLastProject: boolean) => {
	const normalized = clampFallbackDelay(baseDelay);
	if (hasLastProject) {
		return normalized;
	}
	const scaled = Math.round(normalized * 0.4);
	if (scaled < SPLASH_FALLBACK_DELAY_MIN) {
		return SPLASH_FALLBACK_DELAY_MIN;
	}
	if (scaled > SPLASH_FALLBACK_DELAY_MAX) {
		return SPLASH_FALLBACK_DELAY_MAX;
	}
	return scaled;
};
