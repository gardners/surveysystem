
const getGroupId = function(id) {
    // TODO optimize for performance
     const parts = id.split('__');
     if (parts.length > 1) {
         parts.pop();
    }
     return parts.join('__').trim();
};


class DisplayGroups {
    constructor(questions) {
        this.groups = [];

        let lastid = '';
        questions.forEach((q) => {
            const id = getGroupId(q.id);

            // first
            if(id && id !== lastid) {
                this.groups.push([q]);
                lastid = id;
                return;
            }

            //match - middle
            if(id && id === lastid) {
                this.groups[this.groups.length - 1].push(q);
                lastid = id;
                return;
            }

            // anything else
            this.groups.push([q]);

            // cache id
            lastid = id;
        });

        //reset cache
        lastid = '';
    }

}

export { DisplayGroups as default, getGroupId };
