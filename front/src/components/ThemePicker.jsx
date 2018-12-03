//Basic ThemePicker made with bootstrap
import React from 'react';
import PropTypes from 'prop-types';

const ThemePicker = function(props) {
    return (
        <select className={ props.className } onChange={ (e) => {
            e.preventDefault();
            if(!e.target.value) {
                return;
            }

            const url = new URL(window.location);
            url.searchParams.set('theme', e.target.value);
            window.location.href = url.toString();
        } }>
            <option>Pick theme...</option>
            { ['default', 'slate', 'lux', 'simplex', 'superhero', 'united', 'minty'].map((val, index) => <option key={ index } value={ val }>{ val }</option>) }
        </select>
    );
}

ThemePicker.propTypes = {
    className: PropTypes.string,
};

export default ThemePicker;
