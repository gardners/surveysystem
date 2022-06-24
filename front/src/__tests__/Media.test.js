import {  DEFAULT_BREAKPOINT, testMediaBreakpoint, matchesBreakpointOrAbove } from '../Media';

// disable warnings from Media.js
const log = console.log;
console.warn = () => {};

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

// restore logging
console.log = log;
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

    test('DEFAULT_BREAKPOINT', () => {
        expect(DEFAULT_BREAKPOINT).toBe('md');
    });

    test('testMediaBreakpoint', () => {

        // xl

        deviceWidth = 1200; // exact match
        testMediaBreakpoint(bp => expect(bp).toBe('xl'));

        deviceWidth = 1201; // above
        testMediaBreakpoint(bp => expect(bp).toBe('xl'));

        deviceWidth = 1199; // below
        testMediaBreakpoint(bp => expect(bp).toBe('lg'));

        // lg

        deviceWidth = 992; // exact match
        testMediaBreakpoint(bp => expect(bp).toBe('lg'));

        deviceWidth = 993; // above
        testMediaBreakpoint(bp => expect(bp).toBe('lg'));

        deviceWidth = 991; // below
        testMediaBreakpoint(bp => expect(bp).toBe('md'));

        // md

        deviceWidth = 768; // exact match
        testMediaBreakpoint(bp => expect(bp).toBe('md'));

        deviceWidth = 769; // above
        testMediaBreakpoint(bp => expect(bp).toBe('md'));

        deviceWidth = 767; // below
        testMediaBreakpoint(bp => expect(bp).toBe('sm'));

        // sm

        deviceWidth = 576; // exact match
        testMediaBreakpoint(bp => expect(bp).toBe('sm'));

        deviceWidth = 577; // above
        testMediaBreakpoint(bp => expect(bp).toBe('sm'));

        deviceWidth = 575; // below
        testMediaBreakpoint(bp => expect(bp).toBe('xs'));

        // xs

        deviceWidth = 0; // exact
        testMediaBreakpoint(bp => expect(bp).toBe('xs'));

        deviceWidth = 1; // above
        testMediaBreakpoint(bp => expect(bp).toBe('xs'));
    });

    test('matchesBreakpointOrAbove', () => {

        // prepare test, set register to 'md'
        deviceWidth = 769; // one above md
        testMediaBreakpoint(bp => expect(bp).toBe('md'));

        expect(matchesBreakpointOrAbove('xl')).toBe(false);
        expect(matchesBreakpointOrAbove('lg')).toBe(false);
        expect(matchesBreakpointOrAbove('md')).toBe(true);
        expect(matchesBreakpointOrAbove('sm')).toBe(true);
        expect(matchesBreakpointOrAbove('xs')).toBe(true);

    });
/*
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
    */
});
