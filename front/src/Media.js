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

const BreakPoints = [
    ['xs', 0],
    ['sm', 576],
    ['md', 768],
    ['lg', 992],
    ['xl', 1200],
];

/**
 * Checks if a given bootstrap media-query breakpoint applies and returns it.
 *
 * @returns {bool}
 */

const getMediaBreakpoint = function() {
    const { length } = BreakPoints;
    let last = 'xs';
    let point;

    for (let i = 0; i < length; i += 1) {
        point = BreakPoints[i][0];
        if (!matchesBreakpoint(point) ) {
            return last;
        }
        last = point;
    }

    return last;
};

/**
 * Checks if a given bootstrap media-query breakpoint applies and returns it.
 *
 * @returns {bool}
 */

const matchesBreakpoint = function(bp) {
    const hits = BreakPoints.filter(point => point[0] === bp);

    if (!hits.length) {
        return false;
    }

    // IE < 9, react test renderer
    if (typeof matchMedia !== 'function') {
        return false;
    }

    return window.matchMedia(`min-width: ${hits[0][1]}px`).matches;
};


export {
    getMediaBreakpoint,
    matchesBreakpoint,
};
