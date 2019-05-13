import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';

const MenuLink = function({ to, children }) {
    const regex = new RegExp(to);
    const active = regex.test(window.location.pathname);
    return (
        <Link className={ `dropdown-item ${(active) ? ' active' : ''}` } to={ to }>{ children }</Link>
    );
};

MenuLink.propTypes = {
    to: PropTypes.string.isRequired,
};

class DropdownMenu extends React.Component {
    constructor(props){
        super(props);

        this.state = {
            show: false,
        };

        this.show = this.show.bind(this);
        this.hide = this.hide.bind(this);

    };

    show(event) {
        event.preventDefault();
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
                <button className="button" className="btn btn-secondary dropdown-toggle" type="button" aria-haspopup="true" aria-expanded="false"
                    onClick={ this.show }>
                    { this.props.title }
                </button>

                { this.state.show &&
                    <div className={ `dropdown-menu ${cls}` } aria-labelledby="dropdownMenuButton">
                        { this.props.children }
                    </div>
                }
            </div>
        );
    }
}

DropdownMenu.propTypes = {
    title: PropTypes.string.isRequired,
};

export { DropdownMenu, MenuLink };
