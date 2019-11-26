// Math.max(document.documentElement.clientWidth, window.innerWidth || 0);

/**
 * @var Breakpoints A map of bootstrap 4 breakpoints { flag: pixels }
 * @see bootstrapp/scss/mixins/_breakpoints.scss
 * (xs: 0, sm: 576px, md: 768px, lg: 992px, xl: 1200px)
 * @see bootstrap.css :root
 *     --breakpoint-xs: 0;
 *     --breakpoint-sm: 576px;
 *     --breakpoint-md: 768px;
 *     --breakpoint-lg: 992px;
 *     --breakpoint-xl: 1200px;
 */

const DEFAULT_BREAKPOINT = 'md';

const xs = 0;
const sm = 576;
const md = 768;
const lg = 992;
const xl = 1200;

const order = ['xs', 'sm', 'md', 'lg', 'xl'];

let breakpoint = DEFAULT_BREAKPOINT;

const register = function (bp, callback) {
    breakpoint = bp;
    if(typeof callback === 'function') {
        callback(bp);
    }
}

const testMediaBreakpoint = function(callback) {
    // IE < 9, react test renderer
    if (typeof window.matchMedia !== 'function') {
        return 'NONE';
    }

    // test from high to low and register;
    let m;

    m = window.matchMedia(`(min-width: ${xl}px`);
    if (m && m.matches) {
        register('xl', callback);
        return;
    }

    m = window.matchMedia(`(min-width: ${lg}px`);
    if (m && m.matches) {
        register('lg', callback);
        return;
    }

    m = window.matchMedia(`(min-width: ${md}px`);
    if (m && m.matches) {
        register('md', callback);
        return;
    }

    m = window.matchMedia(`(min-width: ${sm}px`);
    if (m && m.matches) {
        register('sm', callback);
        return;
    }

    m = window.matchMedia(`(min-width: ${xs}px`);
    if (m && m.matches) {
        register('xs', callback);
        return;
    }

    console.warn('testMediaBreakpoint: No media query breakpoint found');
};

const matchesBreakpointOrAbove = function(bp) {
    return order.indexOf(breakpoint) >= order.indexOf(bp);
};

export {
    DEFAULT_BREAKPOINT,
    testMediaBreakpoint,
    matchesBreakpointOrAbove,
};
