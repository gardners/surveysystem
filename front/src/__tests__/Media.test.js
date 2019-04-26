import {  getMediaBreakpoint, matchesBreakpoint } from '../Media';

let deviceWidth = 0;

window.matchMedia = jest.fn().mockImplementation(query => {

    let width = -1;
    const hits = query.match(/min-width:\s?(\d+)px/i);
    if(hits) {
        width = parseInt(hits[1], 10);
    }

    return {
        matches: (width <= deviceWidth),
        media: query,
        onchange: null,
        addListener: jest.fn(),
        removeListener: jest.fn(),
    };
});

 /**
  * @see bootstrapp/scss/mixins/_breakpoints.scss
  * (xs: 0, sm: 576px, md: 768px, lg: 992px, xl: 1200px)
  * @see bootstrap.css :root
  *     --breakpoint-xs: 0;
  *     --breakpoint-sm: 576px;
  *     --breakpoint-md: 768px;
  *     --breakpoint-lg: 992px;
  *     --breakpoint-xl: 1200px;
  */

describe('window media API utils', () => {

    test('getMediaBreakpoint', () => {
        deviceWidth = 1200; // exact match
        expect(getMediaBreakpoint()).toBe('xl');
        deviceWidth = 1201; // above
        expect(getMediaBreakpoint()).toBe('xl');
        deviceWidth = 1199; // below
        expect(getMediaBreakpoint()).toBe('lg');


        deviceWidth = 992; // exact match
        expect(getMediaBreakpoint()).toBe('lg');
        deviceWidth = 993; // above
        expect(getMediaBreakpoint()).toBe('lg');
        deviceWidth = 991; // below
        expect(getMediaBreakpoint()).toBe('md');

        deviceWidth = 768; // exact match
        expect(getMediaBreakpoint()).toBe('md');
        deviceWidth = 769; // above
        expect(getMediaBreakpoint()).toBe('md');
        deviceWidth = 767; // below
        expect(getMediaBreakpoint()).toBe('sm');

        deviceWidth = 576; // exact match
        expect(getMediaBreakpoint()).toBe('sm');
        deviceWidth = 577;
        expect(getMediaBreakpoint()).toBe('sm');
        deviceWidth = 575; // below
        expect(getMediaBreakpoint()).toBe('xs');

        deviceWidth = 0;
        expect(getMediaBreakpoint()).toBe('xs');
    });

    test('matchesBreakpoint', () => {
        expect(matchesBreakpoint('invalid')).toBe(false);

        deviceWidth = 1200; // exact match
        expect(matchesBreakpoint('xl')).toBe(true);
        deviceWidth = 1201; // above
        expect(matchesBreakpoint('xl')).toBe(true);
        deviceWidth = 1199; // below
        expect(matchesBreakpoint('xl')).toBe(false);

        deviceWidth = 992; // exact match
        expect(matchesBreakpoint('lg')).toBe(true);
        deviceWidth = 993; // above
        expect(matchesBreakpoint('lg')).toBe(true);
        deviceWidth = 991; // below
        expect(matchesBreakpoint('lg')).toBe(false);

        deviceWidth = 768; // exact match
        expect(matchesBreakpoint('md')).toBe(true);
        deviceWidth = 769; // above
        expect(matchesBreakpoint('md')).toBe(true);
        deviceWidth = 767; // below
        expect(matchesBreakpoint('md')).toBe(false);

        deviceWidth = 576; // exact match
        expect(matchesBreakpoint('sm')).toBe(true);
        deviceWidth = 577;
        expect(matchesBreakpoint('sm')).toBe(true);
        deviceWidth = 575; // below
        expect(matchesBreakpoint('sm')).toBe(false);

        deviceWidth = 0;
        expect(matchesBreakpoint('xs')).toBe(true);
    });

    test('No media Api support', () => {
        window.matchMedia = null;
        deviceWidth = 992; // exact match 'lg'
        expect(matchesBreakpoint('invalid')).toBe(false);
        expect(matchesBreakpoint('lg')).toBe(false);
        expect(matchesBreakpoint('xs')).toBe(false);
    });
});
