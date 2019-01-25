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

const BreakPoints = Object.freeze({
    xs: 0,
    sm: 576,
    md: 768,
    lg: 992,
    xl: 1200,
});

/**
 * Checks if a given bootstrap media-query breakpoint applies and returns it.
 *
 * @returns {string}
 */
const getMediaBreakPoint = function() {

    let breakpoint = 'xl';
    let last = 'xs';

    for (let flag in BreakPoints) {
        if (!BreakPoints.hasOwnProperty(flag)) {
            continue;
        }
        const { matches } = matchMedia(`(min-width: ${BreakPoints[flag]}px`);

        if (!matches) {
            return flag;
        }

        last = flag;
    }
    return 'xl';
};

export {
    getMediaBreakPoint,
};
