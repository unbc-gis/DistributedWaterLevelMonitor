"""empty message

Revision ID: 62a7bb357ca7
Revises: 402a3ceacf73
Create Date: 2020-04-15 10:31:32.053752

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = '62a7bb357ca7'
down_revision = '402a3ceacf73'
branch_labels = None
depends_on = None


def upgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    #op.drop_table('spatial_ref_sys')
    #op.drop_index('idx_deployment_location', table_name='deployment')
    op.add_column('measurement', sa.Column('imei', sa.Text(), nullable=True))
    op.add_column('measurement', sa.Column('lat', sa.Numeric(), nullable=True))
    op.add_column('measurement', sa.Column('lon', sa.Numeric(), nullable=True))
    # ### end Alembic commands ###


def downgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.drop_column('measurement', 'lon')
    op.drop_column('measurement', 'lat')
    op.drop_column('measurement', 'imei')
    op.create_index('idx_deployment_location', 'deployment', ['location'], unique=False)
    op.create_table('spatial_ref_sys',
    sa.Column('srid', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.Column('auth_name', sa.VARCHAR(length=256), autoincrement=False, nullable=True),
    sa.Column('auth_srid', sa.INTEGER(), autoincrement=False, nullable=True),
    sa.Column('srtext', sa.VARCHAR(length=2048), autoincrement=False, nullable=True),
    sa.Column('proj4text', sa.VARCHAR(length=2048), autoincrement=False, nullable=True),
    sa.CheckConstraint('(srid > 0) AND (srid <= 998999)', name='spatial_ref_sys_srid_check'),
    sa.PrimaryKeyConstraint('srid', name='spatial_ref_sys_pkey')
    )
    # ### end Alembic commands ###