import React, { Component } from 'react';
import PropTypes from 'prop-types';
import { NavLink } from 'react-router-dom';

import { Fade } from '../Transitions';

const MenuLink = function({ to, children }) {
    return (
        <NavLink exact className="dropdown-item" to={ to }>{ children }</NavLink>
    );
};

MenuLink.propTypes = {
    to: PropTypes.string.isRequired,
};

class DropdownMenu extends Component {
    constructor(props){
        super(props);

        this.state = {
            show: false,
        };

        this.show = this.show.bind(this);
        this.hide = this.hide.bind(this);

    };

    show(e) {
        e && e.preventDefault()
        this.setState({ show: true }, () => {
            document.addEventListener('click', this.hide);
        });
    }

    hide() {
        this.setState({ show: false }, () => {
            document.removeEventListener('click', this.hide);
        });
    }

    render() {
        const cls = (this.state.show) ? 'show' : '';

        return (
            <div className={ `dropdown ${cls}` }>
                <button className={ `${this.props.buttonClassName}  dropdown-toggle` } type="button" aria-haspopup="true" aria-expanded="false"
                    onClick={ this.show }>
                    { this.props.title }
                </button>

                { this.state.show &&
                    <Fade timeout={ 100 }>
                        <div className={ `dropdown-menu ${cls}` } aria-labelledby="dropdownMenuButton">
                            { this.props.children }
                        </div>
                    </Fade>
                }
            </div>
        );
    }
}

DropdownMenu.defaultProps = {
    buttonClassName: 'btn btn-secondary',
};

DropdownMenu.propTypes = {
    title: PropTypes.string.isRequired,
    buttonClassName: PropTypes.string,
};

export {
    DropdownMenu,
    MenuLink
};
