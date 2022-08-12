import React, { useState } from 'react';
import PropTypes from 'prop-types';
import { NavLink } from 'react-router-dom';

import { addClassNames } from '../../Utils';
import { Fade } from '../Transitions';


const MenuLink = function({ to, children }) {
    return (
        <NavLink exact className="dropdown-item" to={ to }>{ children }</NavLink>
    );
};

MenuLink.propTypes = {
    to: PropTypes.string.isRequired,
};


const DropdownMenu = function({ title, buttonClass, alignmentClass, children }) {
    const [show, setShow] = useState(false);

    const toggled = (show) ? 'show' : '';
    const onclick = function(e) {
        e.preventDefault();
        setShow(!show);
    };

    return (
        <div className={ addClassNames('dropdown', toggled) }>
            <button
                type="button"
                className={ addClassNames(buttonClass, 'dropdown-toggle') }
                onClick={ onclick }
                aria-haspopup="true"
                aria-expanded="false"
            >
                { title }
            </button>
            { show &&
                <Fade timeout={ 100 }>
                    <div className={ addClassNames('dropdown-menu', toggled, alignmentClass) }>
                        { children }
                    </div>
                </Fade>
            }
        </div>
    );
};

DropdownMenu.defaultProps = {
    buttonClass: 'btn btn-secondary',
    alignmentClass: '',
};

DropdownMenu.propTypes = {
    title: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.element,
    ]).isRequired,
    buttonClass: PropTypes.string,
    alignmentClass: PropTypes.string,
};


export {
    DropdownMenu,
    MenuLink
};
